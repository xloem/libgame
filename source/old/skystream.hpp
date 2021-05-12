#pragma once

#include <chrono>
#include <thread>
#include <unordered_map>

// iostreams for debug
//#include <iostream>

#include <nlohmann/json.hpp>

#include "portalpool.hpp"

#include "crypto.hpp"

/*
 * I think the algorithm for redepthing the trees may have been designed in error.
 * i'm somewhat confused.  it's hard to consider whether or not the trees become roughly balanced.
 */

/*
 * . later: lookup nodes may shift (or stretch) data by specifying a subspan within.
 *          for now, they assume spans are in the same coordinate space.
 */

/*
 * In the case of append-only, this simplifies to the same dat-inspired algorithm.
 *
 * TODO after: make an interface function that provides for many random writes at once, and
 * store them consolidated.
 *
 * It would be fine, afterwards, to offer a block device with fuse that has size equal to the
 * full integer extent of the offset datatype.  You could also make data isomorphically
 * interchangeable with a git repository.
 */

using seconds_t = double;

seconds_t time()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / double(1000000);
}

class skystream
{
public:
	skystream(sia::portalpool & portalpool, std::string way, std::string link)
	: portalpool(portalpool)
	{
		std::vector<uint8_t> data;
		tail.metadata = get_json({{way,link}});
		tail.identifiers = cryptography.digests({&data});
		tail.identifiers[way] = link;
	}
	skystream(sia::portalpool & portalpool, nlohmann::json identifiers = {})
	: portalpool(portalpool)
	{
		tail.identifiers = identifiers;
		if (!identifiers.empty()) {
			tail.metadata = get_json(identifiers);
		} else {
			auto now = time();
			tail.metadata = {
				{"content", {
					{"spans",{
						//{"real", {
							{"time", {{"start", now}, {"end", now}}},
							{"index", {{"start", 0}, {"end", 0}}},
							{"bytes", {{"start", 0}, {"end", 0}}}
						//}}
					}}
				}},
				{"lookup", nlohmann::json::array()}//,
				//{"flows", {}}
				
			};
		}
	}

	skystream(skystream const &) = default;
	skystream(skystream &&) = default;

	std::vector<uint8_t> read(std::string span, double & offset, std::string flow = "real", nlohmann::json * user_metadata = nullptr, sia::portalpool::worker const * worker = 0)
	{
		(void)flow;
		auto metadata = this->get_node(tail, span, offset, {}, false, worker).metadata;
		std::lock_guard<std::mutex> lock(methodmtx);
		auto metadata_content = metadata["content"];
		double content_start = metadata_content["spans"][span]["start"];
		if (span != "bytes" && offset != content_start) {
			throw std::runtime_error("ambiguous middle read: " + span + " " + std::to_string(offset) + " is within block span");
		}
		std::vector<uint8_t> data;
		auto metadata_content_bytes = metadata_content["spans"]["bytes"];
		if (metadata_content_bytes["start"] != metadata_content_bytes["end"]) {
			data = get(metadata_content["identifiers"], worker);
		}
	
		auto begin = data.begin() + offset - content_start;
		// the goal here was, if the span is bytes, to use it as the offset in
		// otherwise, to just return the whole chunk
		auto end = begin;
		if (span == "bytes") {
			end += (uint64_t)metadata_content["bounds"]["bytes"]["end"] - content_start;
		} else {
			end = data.end();
		}
		offset = metadata_content["bounds"][span]["end"];
		if (nullptr != user_metadata) {
			*user_metadata = metadata["metadata"];
		}
		return {begin, end};
	}

	nlohmann::json user_metadata(std::string span, double & offset, std::string flow = "real", sia::portalpool::worker const * worker = 0)
	{
		nlohmann::json metadata;
		read(span, offset, flow, &metadata, worker);
		return metadata;
	}
	// oops, this goes elsewhere: "people don't gain much from making torture communities,"
	//   "huge groups of people just experiencing punishment to validate punishing."
	//   "such patterns don't produce anything for a community, themselves, or an individual"
	//   "so they tend to fade away eventually."
	//   		[we respond to threats of punishment minimally, in the hope of reconnecting
	//   		 with our punishers or our habits of them in us, who know how to end it.]

	std::mutex writemtx;
	void write(std::vector<uint8_t> const & data, std::string span, double offset, std::map<std::string, std::pair<double,double>> const & custom_spans = {}, nlohmann::json const & user_metadata = {}, sia::portalpool::worker const * worker = 0)
	{
		if (data.empty()) {
			throw std::logic_error("todo: verify lookup nodes are consistent when some have no bytes");
		}
		std::lock_guard<std::mutex> writelock(writemtx);
		bool extra_leaf = false;

		// the current top node is this->tail
		
		std::unique_lock<std::mutex> lock(methodmtx);
		seconds_t end_time = time();
		seconds_t start_time = tail.metadata["content"]["spans"]["time"]["end"];

		unsigned long long start_bytes;
		// calculate start index (index always increments);
		unsigned long long index = tail.metadata["content"]["spans"]["index"]["end"];
		// calculate start bytes
		// TODO: these cases can be merged by catching out of bounds for head node
		if (offset == tail.metadata["content"]["spans"][span]["end"]) {
			// append case, no head node to replace
			start_bytes = tail.metadata["content"]["spans"]["bytes"]["end"];
			extra_leaf = true;
		} else {
			// non-append: find head node
			auto & head_node = get_node(this->tail, span, offset, {}, false, worker);
			auto head_node_content = head_node.metadata["content"];
			double start_head = head_node_content["bounds"][span]["start"];
			if (span == "bytes") {
				start_bytes = offset;
				if (offset != start_head) {
					extra_leaf = true;
				}
			} else {
				if (offset != start_head) {
					// throw here because we don't know how spans might interpolate inside data.  user would have to provide all spans.
					throw std::runtime_error(span + " " + std::to_string(offset) + " is within block span");
				}
				start_bytes = head_node_content["bounds"]["bytes"]["start"]; 
			}
		}
		// calculate end bytes
		unsigned long long end_bytes = start_bytes + data.size(); /*full_size*/
		// these are the spans of the new write, for now
		nlohmann::json spans = {
			{"time", {{"start", start_time},{"end", end_time}}},
			{"bytes", {{"start", start_bytes},{"end", end_bytes}}},
			{"index", {{"start", index}, {"end", index + 1}}}
		};
		for (auto & span : custom_spans) {
			if (!spans.contains(span.first)) {
				spans[span.first] = {{"start", span.second.first}, {"end", span.second.second}};
			}
		}

		try {
			auto & tail_node = get_node(tail, "bytes", end_bytes, {}, false, worker);
			auto tail_node_content = tail_node.metadata["content"];
			if (end_bytes != tail_node_content["bounds"]["bytes"]["start"]) {
				extra_leaf = true;
			}
		} catch (std::out_of_range const &) {
			extra_leaf = true;
		}
		
		/* find the desired tree depth */
		size_t depth = 0;
		nlohmann::json old_lookup_nodes = tail.metadata["lookup"];
		bool full_tree = old_lookup_nodes.empty() ? true : false;
		for (auto & node : old_lookup_nodes) {
			auto subdepth = node["depth"];
			if (subdepth > depth) {
				depth = subdepth;
				full_tree = false;
			} else if (subdepth == depth) {
				full_tree = true;
			}
		}

		/* insert tail lookup node */
		old_lookup_nodes = node_with_lookup(tail, 0);

		/* increment depth if tree is full and leaf is added.
		 * a leaf is not added when a block is precisely overwritten.
		 */
		if (full_tree && extra_leaf) {
			++ depth;
		}

		/* redepth / rebalance the nodes */
		nlohmann::json new_lookup_nodes = redepthed_lookup(old_lookup_nodes, depth, tail, worker);
		old_lookup_nodes = new_lookup_nodes;
		new_lookup_nodes = nlohmann::json::array();

		/* break existing lookup nodes around the data */
		nlohmann::json prev_bound;
		for (auto & new_data_span :  {spans, nlohmann::json()}) {
				/* if adding multiple data, move content to within lookup nodes for simplicity */
			auto sublookup = sliced_lookup(old_lookup_nodes, prev_bound, new_data_span);
			// this is where the lookup lists are concatenated all together around middle writes.
			// this means checking for continuity needs to involve attached data.
			for (auto & lookup_item : sublookup) {
				new_lookup_nodes[new_lookup_nodes.size()] = lookup_item;
			}
			prev_bound = new_data_span;
		}
					// the below plan was deprioritised in favor of middle
					// writes.  the plan involved tracking different "flows"
					// so that data could have different parallel orderings,
					// e.g. a logical and a time-wise ordering might be in
					// different flows.
					// 10: we're changing the format to use "flows" of "real" and "logic" as below
					// 		[this change is at the edge of checks for likely-to-finish-task.  this is known.
					// 		 so, no more generalization until something is working and usable.]
					// 		[ETA for completion has doubled.]
					// 			[we have other tasks we want to do.]
					// 				[considering undoing proposed change.]
					// 					[okay, demand-to-make-more-general: you don't seem to have the capacity to support
					// 					 that.  i am forcing you to not have it be more general.]
					// 		[we will implement a test before changing further.  thank you all for the relation.]
					// {
					//	'sia-skynet-stream': "1.1.0_debugging",
					// 	content: {
					// 		identifiers: {important-stuff},
					// 		spans: {
					// 			{real: {time:}},
					// 			{logic: {bytes:,index:}}
					// 		}
					//	},
					//	flows: { // 'lookup' gets translated to 'flows'
					//		real: [
					//			{
					//				identifiers: {important-stuff},
					//	 			spans: {
					//	 				{real: {time:}},
					//	 				{logic: {bytes:,index:}}
					//	 			}
					//			}, ...
					//		], // we're considering updating this to not store multiple copies of the lookup data for each order.
					//		   // but we're noting with logic-space changes, the trees may refer to different nodes.  maybe leave for later.
					//		   	// for streams, reuse is helpful.
					//		logic: [
					//			{
					//				identifiers: {important-stuff},
					//	 			spans: {
					//	 				{real: {time:}},
					//	 				{logic: {bytes:,index:}}
					//	 			}
					//			}, ...
					//		]
					//	}
					//}

					// NOTE: we need to update identifiers of lookup_nodes to point to something that contains both
					//
		// 9: this is old original implementation, below loop, before multiple rewrites
		/*
		while (lookup_nodes.size() && lookup_nodes.back()["depth"] == depth) {
			auto & back = lookup_nodes.back();
			auto & back_spans = back["spans"];
			for (auto & span : new_lookup_node["spans"].items()) {
				auto start = back_spans[span.key()]["start"];
				span.value()["start"] = start;
			}
			auto end = lookup_nodes.end();
			-- end;
			lookup_nodes.erase(end);

			++ depth;
		}
		*/


		// end: we can make a new tail metadata node that indexes everything afterward.  it can even have tree nodes if desired.
		// 5: remaining before testing: build lookup nodes using three more sources in 1-2-3 order

		auto content_identifiers = cryptography.digests({&data});
		nlohmann::json metadata_json = {
			{"sia-skynet-stream", "1.0.12"},
			{"content", {
				{"spans", spans},
				{"identifiers", content_identifiers},
			}},
			/* 10:
			{"flow", { 
				{"logical", new_lookup_nodes},
				{"creation", append_only_lookup_nodes_of_time_and_index}
			}},
			*/
			{"lookup", new_lookup_nodes}
		};
		if (! user_metadata.is_null()) {
			metadata_json["metadata"] = user_metadata;
		}
		std::string metadata_string = metadata_json.dump();
		//std::cerr << metadata_string << std::endl;

		sia::skynet::upload_data metadata_upload("metadata.json", std::vector<uint8_t>{metadata_string.begin(), metadata_string.end()}, "application/json");
		sia::skynet::upload_data content("content", data, "application/octet-stream");

		// note: the numbered changes scattered around are left over from the flows idea.
		// keeping them is only as a reminder to make it stable to have spans with
		// different ordering.  this would be clearer and computer-checkable if the "flows"
		// syntax were used.

		// CHANGE 3C: let's try to reuse all surrounding data using the new 'bounds' attribute
		// 3C: TODO: we want to insert into content from head_node if we are doing a midway-write (full_size above).  we could also split the write into two.

		auto metadata_identifiers = cryptography.digests({&metadata_upload.data});

		lock.unlock();

		std::mutex skylink_mutex;
		std::string skylink;
		auto ensure_upload = [&]() {
			std::string link = portalpool.upload(metadata_identifiers["sha3_512"], {metadata_upload, content}, false, worker);
			{
				std::lock_guard<std::mutex> lock(skylink_mutex);
				skylink = link;
			} 
		};
		auto upload1 = std::thread(ensure_upload);
		auto upload2 = std::thread(ensure_upload);
		upload1.join();
		upload2.join();
		lock.lock();
		metadata_identifiers["skylink"] = skylink + "/" + metadata_upload.filename;
		metadata_json["content"]["identifiers"]["skylink"] = skylink + "/" + content.filename;

		// if we want to support threading we'll likely need a lock around this whole function (not just the change to tail)
		// 	later: i've done that, but haven't integrated with old stuff to simplify
		tail.identifiers = metadata_identifiers;
		tail.metadata = metadata_json;
	}

	std::map<std::string,std::pair<double,double>> block_spans(std::string span, double offset, bool get_preceding = false, sia::portalpool::worker const * worker = 0)
	{
		std::lock_guard<std::mutex> lock(methodmtx);
		auto metadata = this->get_node(tail, span, offset, {}, get_preceding, worker).metadata;
		std::map<std::string,std::pair<double,double>> result;
		for (auto & content_span : metadata["content"]["spans"].items()) {
			auto span = content_span.key();
			result[span].second = content_span.value()["end"];
			result[span].first = content_span.value()["start"];
		}
		return result;
	}

	std::pair<double,double> block_span(std::string span, double offset, sia::portalpool::worker const * worker = 0)
	{
		return block_spans(span, offset, worker)[span];
	}

	std::map<std::string,std::pair<double,double>> spans()
	{
		std::lock_guard<std::mutex> lock(methodmtx);
		std::map<std::string,std::pair<double,double>> result;
		for (auto & content_span : tail.metadata["content"]["spans"].items()) {
			auto span = content_span.key();
			result[span].second = content_span.value()["end"];
			result[span].first = content_span.value()["start"];
		}
		for (auto & lookup : tail.metadata["lookup"]) {
			for (auto & lookup_span : lookup["spans"].items()) {
				auto span = lookup_span.key();
				double point = lookup_span.value()["start"];
				if (point < result[span].first) {
					result[span].first = point;
				}
			}
		}
		return result;
	}

	std::pair<double,double> span(std::string span)
	{
		return spans()[span];
	}

	std::map<std::string,double> lengths()
	{
		std::map<std::string,double> result;
		for (auto & span : spans()) {
			result[span.first] = span.second.second - span.second.first;
		}
		return result;
	}

	double length(std::string span)
	{
		double spanlength = lengths()[span];
		return spanlength;
	}

	nlohmann::json identifiers()
	{
		std::lock_guard<std::mutex> lock(methodmtx);
		return tail.identifiers;
	}

	nlohmann::json identifiers(std::string span, double offset, bool get_preceding = false)
	{
		return get_node(tail, span, offset, {}, get_preceding).identifiers;
	}

	std::vector<uint8_t> get(nlohmann::json identifiers, sia::portalpool::worker const * worker = 0)
	{
		auto skylink = identifiers["skylink"];
		std::vector<uint8_t> result = portalpool.download(skylink, {}, 1024*1024*64, false, worker).data;
		auto digests = cryptography.digests({&result});
		for (auto & digest : digests.items()) {
			if (identifiers.contains(digest.key())) {
				if (digest.value() != identifiers[digest.key()]) {
					throw std::runtime_error(digest.key() + " digest mismatch.  identifiers=" + identifiers.dump() + " digests=" + digests.dump());
				}
			}
		}
		return result;
	}

protected:
	std::mutex methodmtx;
	sia::portalpool & portalpool;

private:
	struct node
	{
		nlohmann::json identifiers;
		nlohmann::json metadata;
	};

	node & get_node(node & start, std::string span, double offset, nlohmann::json bounds = {}, bool get_preceding = false, sia::portalpool::worker const * worker = 0)
	{
		auto content_spans = start.metadata["content"]["spans"];
		auto content_span = content_spans[span];
		std::cout << "get_node " << span << " " << content_span << " " << start.metadata["content"]["identifiers"]["skylink"] << std::endl;
		if (get_preceding
			? (offset > content_span["start"] && offset <= content_span["end"])
			: (offset >= content_span["start"] && offset < content_span["end"])
		) {
			start.metadata["content"]["bounds"] = bounds.is_null() ? content_spans : bounds;
			return start;
		}
		for (auto & lookup : start.metadata["lookup"]) {
			auto lookup_spans = lookup["spans"];
			for (auto & bound : bounds.items()) {
				if (!lookup_spans.contains(bound.key())) { continue; }
				auto bound_span = lookup_spans[bound.key()];
				if (bound.value()["begin"] > bound_span["begin"]) {
					bound_span["begin"] = bound.value()["begin"];
				}
				if (bound.value()["end"] < bound_span["end"]) {
					bound_span["end"] = bound.value()["end"];
				}
			}
			auto lookup_span = lookup["spans"][span];
			double start = lookup_span["start"];
			double end = lookup_span["end"];
			if (get_preceding
				? (offset > start && offset <= end)
				: (offset >= start && offset < end)
			) {
				auto identifiers = lookup["identifiers"];
				return get_node(cached_node(identifiers, worker), span, offset, lookup_spans, get_preceding, worker);
			}
		}
		throw std::out_of_range(span + " " + std::to_string(offset) + " out of range");
	}

	nlohmann::json get_json(nlohmann::json identifiers, std::vector<uint8_t> * data = nullptr, sia::portalpool::worker const * worker = 0)
	{
		auto data_result = get(identifiers, worker);
		if (data) { *data = data_result; }
		auto result = nlohmann::json::parse(data_result);
		// TODO improve (refactor?), hardcodes storage system and is slow due to 2 requests for each chunk
		std::string skylink = identifiers["skylink"];
		skylink.resize(52); skylink += "/content";
		result["content"]["identifiers"]["skylink"] = skylink;
		return result;
	}

	/*** todo later: this would be simpler if all the data content were included in the lookup nodes list.  this special handling then might not be needed. ***/
	nlohmann::json node_with_lookup(node const & additional_node, size_t node_depth)
	{			/* node_depth is 1 + max depth of lookup nodes of this node */
		bool inserted = false;
		nlohmann::json additional_item = {
			{"identifiers", additional_node.identifiers},
			{"spans", additional_node.metadata["content"]["spans"]},
			{"depth", node_depth}
		};
		nlohmann::json new_lookup_nodes = nlohmann::json::array();
		for (auto & node : additional_node.metadata["lookup"]) {
			if (node["spans"]["bytes"]["start"] == additional_node.metadata["content"]["spans"]["bytes"]["end"]) {
				new_lookup_nodes[new_lookup_nodes.size()] = additional_item;
				inserted = true;
			}
			new_lookup_nodes[new_lookup_nodes.size()] = node;
		}
		if (!inserted) {
			new_lookup_nodes[new_lookup_nodes.size()] = additional_item;
		}
		return new_lookup_nodes;
	}

	nlohmann::json sliced_lookup(nlohmann::json const & old_list, nlohmann::json const & prev_spans = {}, nlohmann::json const & next_spans = {})
	{
		nlohmann::json new_list = nlohmann::json::array();
		for (nlohmann::json const & item : old_list) {
			nlohmann::json new_spans;
			for (auto & span_pair : item["spans"].items()) {
				auto & name = span_pair.key();
				auto & span = span_pair.value();
				auto prev_span = prev_spans.is_null() ? nlohmann::json{} : prev_spans[name];
				auto next_span = next_spans.is_null() ? nlohmann::json{} : next_spans[name];
				nlohmann::json new_span = {
					{"start", span["start"]},
					{"end", span["end"]}
				};
				if (next_span.is_null() && prev_span.is_null()) {
					// if span is not in "next" nor in "prev" then i suppose we discard since multiple slicing could overlap
					continue;
				}
				if (!prev_span.is_null()) {
					// we discard things that have their "end" prior to prev's "end"
					if (span["end"] <= prev_span["end"]) {
						new_spans = {}; break;
					}
					// if "start" is prior to prev's "end", we trim "start"
					if (span["start"] < prev_span["end"]) {
						new_span["start"] = prev_span["end"];
					}
				}
				if (!next_span.is_null()) {
					// we discard things that have their "start" after to next's "start"
					if (span["start"] >= next_span["start"]) {
						new_spans = {}; break;
					}
					// if "end" is after next's "start", we trim "end"
					if (span["end"] > next_span["start"]) {
						new_span["end"] = next_span["start"];
					}
				}
				new_spans[name] = new_span;
			}
			if (! new_spans.is_null()) {
				new_list[new_list.size()] = {
					{"identifiers", item["identifiers"]},
					{"depth", item["depth"]},
					{"spans", new_spans}
				};
			}
		}
		return new_list;
	}

		// ===================================
		/* it's looking like old_list will go away and be retrieved from wrapping_tail.
		 * notably wrapping_tail also includes the area that old_list leaves out, in
		 * its content. */
		// ===================================
	nlohmann::json redepthed_lookup(nlohmann::json const & old_list, size_t stored_depth, node const & wrapping_tail, sia::portalpool::worker const * worker = 0)
	{
		// hopefully this algorithm is improvable if needed
		// we are always rereferencing the top node or copying straight in, but maybe middle nodes could be referenced instead to reduce total size and depth.  maybe this is appropriate when neighbor nodes are contained within the same outer node.
		
		nlohmann::json new_list = nlohmann::json::array();
		for (auto & item : old_list) {
			size_t itemdepth = item["depth"];
			if (itemdepth < stored_depth && item["identifiers"] != wrapping_tail.identifiers) {
				/* todo? this should probably go in the condensing loop.
				 * then depth can be compared with neighbor.  if two neighbors
				 * have differing depths, one of them could be possibly shallowed or
				 * deepened to unite them.
				 */
				/* todo? to reduce deepening, maybe this case should go in the
				 * condensing loop below, and only replace if it is condensable.
				 * or maybe it would be simpler to tag it as re-expandable, and
				 * then re-expand it in another loop for now. */
					/* basically there are some nodes that can be raised
					 * or lowered in order to unit them with neighboring
					 * nodes. this is not being done. when united, we
					 * want them wrapped by tail nodes to do so.
					 * otherwise, we want them shallow for speedy access.
					 */
				/* there's a clear algorithm here, maybe think about in
				 * spare time.  how to unite these nodes in a moving tree,
				 * basically to rebalance the tree. */
				new_list[new_list.size()] = {
					{"identifiers", wrapping_tail.identifiers },
					{"depth", itemdepth + 1},
					{"spans", item["spans"]}
				};
			} else if (itemdepth > stored_depth) {
				auto & itemnode = cached_node(item["identifiers"]);
				nlohmann::json prev_bound, next_bound;
				for (auto & span : item["spans"].items()) {
					prev_bound[span.key()]["end"] = span.value()["start"];
					next_bound[span.key()]["start"] = span.value()["end"];
				}
				auto subnodes = redepthed_lookup(
					sliced_lookup(
						node_with_lookup(itemnode, 0),
						prev_bound,
						next_bound
					),
					stored_depth,
					itemnode,
					worker
				);
				for (auto const & subitem : subnodes) {
					new_list[new_list.size()] = subitem;
				}
			} else {
				new_list[new_list.size()] = item;
			}
		}
		nlohmann::json condensed_list = nlohmann::json::array();
		for (auto & next : new_list) {
			if (!condensed_list.empty()) {
				auto & prev = condensed_list.back();
				if (prev["identifiers"] == next["identifiers"]) {
					size_t prevdepth = prev["depth"];
					size_t nextdepth = next["depth"];
					prev["depth"] = prevdepth > nextdepth ? prevdepth : nextdepth;
					for (auto & span : next["spans"].items()) {
						if (prev["spans"].contains(span.key())) {
							auto & prev_end = prev["spans"][span.key()]["end"];
							auto & next_end = span.value()["end"];
							auto & next_start = span.value()["start"];
							assert (prev_end == next_start);
							prev_end = next_end;
						} else {
							prev["spans"][span.key()] = span.value();
						}
					}
					continue;
				}
			}
			condensed_list[condensed_list.size()] = next;
		}
		return condensed_list;
	}

	node & cached_node(nlohmann::json const & identifiers, sia::portalpool::worker const * worker = 0)
	{
		std::string identifier = identifiers.begin().value();
		if (!cache.count(identifier)) {
			cache[identifier] = node{identifiers, get_json(identifiers, nullptr, worker)};
			std::cout << "cache node " << identifier << ": " << cache[identifier].metadata["content"]["identifiers"]["skylink"] << std::endl;
		}
		return cache[identifier];
	}

	crypto cryptography;
	node tail;
	std::unordered_map<std::string, node> cache;
};

/*
so: mid-stream writing.
we'll want to write a lookup list that lets us find all the stuff after the stream.
let's imagine a use-case scenariio where we are always mid-stream writing.
we'll want to reuse tree roots to find data.

	we have an option of reusing tree roots that have the wrong history.
	this would expand the meaning of 'span' to mean that things outside that span, that _could_ be found from the reference, are just plain wrong.
	this would save us a lot of space and time wrt metadata, so let's do it.

		that means that all the tail links that are after our range can be reused.
 
			then we have a range of stuff that is after our write, but before the last accurate chunk.
				the content documents of most of them are accurate, so we can build 1 or more new chunks to wrap those
	okay, so considering spans as truncations makes most of it easy.
	then, we have one document where only part of the data is accurate.
	what we were planning to do at the start was to extend our data and reupload it.
		let's consider not reuploading unmodified data a bit.

		since the system could handle hyperlarge chunks, it would be good reuse existing data.
		we would shrink the span.
		we would have to modify fucntions to handle reading the spans properly.
			this means tracing the flow of read into subspans.


			okay we're working on reuse of mid-data
				->problem: if we adjust thse spans in the content field, the document will not read right.  we'd need to add an offset, right?
					When reading we _have_ an offset.  we just need to get the read length right.  It doesn't seem that hard.
						we'll also retrieving spans
							[NOTE: WE ARE NOT INSERTING.  ONLY REPLACING.  SO OFFSETS STAY ACCURATE.]
							let's store this as a TODO for now.  reading more important than writing.
								okay considering this further.  we could have a content-type specificaly for this situation.
								it references content from another document, by an offsset.
								it would be a modification to the {"content", {}} structure.
								Maybe it could be a bare field, "byte-offset", "byte-length"
									this would mean changing the read fucntion, but not by very much

										we could make bare metadata documents
										or we could put the content reference in the lookup table
											maybe we could do span-limiting from the lookup table only if depth=0
												yeah i think that would work [small chance it won't]
												it means read() needs to process spans from the deepest lookup
												table, not the metadata associated with the document.
			rewriting get_node to respect bounds.
note: when get-node is called first, it doesn't need to respect any bounds.  there is no larger context.
		[but note that from write() it is called on its own return value to find preceding node from head_node, a middle-node]
			[possible edge cases might be fixable by mutating the cache when we adjust bounds]
 */
