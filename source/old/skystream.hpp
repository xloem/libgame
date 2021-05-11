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
 * The way to do random writes is to reference the previous tree as underlying data,
 * and then include metadata for all the blocks that cannot be accessed in the number of
 * requests equal to the height of the tree.  This means iterating through all the chunks, and
 * it means outputting data with holes in it.  Iteration is sped by using depth attribute to skip.
 *
 * The tree is deepened when both the leaf count increases, and the last tail was balanced:
 * i.e. its lookup_nodes list had two disparate nodes each with the same depth one less than its.
 * The leaf count increases when the data written does not precisely align with a previous chunk.
 *
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
				}}//,
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
			throw std::runtime_error(span + " " + std::to_string(offset) + " is within block span");
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
		std::lock_guard<std::mutex> writelock(writemtx);

		// the current top node is this->tail
		
		std::unique_lock<std::mutex> lock(methodmtx);
		seconds_t end_time = time();
		seconds_t start_time = tail.metadata["content"]["spans"]["time"]["end"];
		
		// for the case of middle-writing, head_node is the node containing the start point
		node * head_node; // contains start point
		//node * outer_head_node; // precedes start point
		// head_bounds stores the bounds of the head node, with bytes shifted to accommodate added data
		nlohmann::json head_bounds;
		unsigned long long start_bytes;
		// calculate start index (index always increments)
		unsigned long long index = tail.metadata["content"]["spans"]["index"]["end"];
		// calculate start bytes
		// TODO: these cases can be merged by catching out of bounds for head node
		if (offset == tail.metadata["content"]["spans"][span]["end"]) {
			// append case, no head node to replace
			start_bytes = tail.metadata["content"]["spans"]["bytes"]["end"];
			head_node = &tail;
			//outer_head_node = &tail;
			head_bounds = head_node->metadata["content"]["bounds"];
			//full_size = data.size();
		} else {
			// non-append: find head node

			nlohmann::json head_node_bounds;
			head_node = &get_node(this->tail, span, offset, {}, false, worker);
			auto head_node_content = head_node->metadata["content"];
			double start_head = head_node_content["bounds"][span]["start"];

			if (span == "bytes") {
				start_bytes = offset;
			} else {
				if (offset != start_head) {
					// throw here because we don't know how spans might interpolate inside data.  user would have to provide all spans.
					throw std::runtime_error(span + " " + std::to_string(offset) + " is within block span");
				}
				start_bytes = head_node_content["bounds"]["bytes"]["start"]; 
			}

			// fill head_bounds, adjusting bytes to remove the bit we replaced.
			for (auto bound : head_node_content["bounds"].items()) {
				if (bound.key() == "bytes") {
					head_bounds["bytes"] = {{"start", bound.value()["start"]},{"end", start_bytes}};
				} else {
					head_bounds[bound.key()] = {{"start", bound.value()["start"]},{"end", bound.value()["end"]}};
				}
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
		// for the case of middle-writing, tail_node is the node containing the end point
		node * tail_node;
		// tail_bounds stores the bounds of the tail node, with bytes shifted to accommodate added data
		nlohmann::json tail_bounds;
		try {
			tail_node = &get_node(tail, "bytes", end_bytes, {}, false, worker);
			auto tail_node_content = tail_node->metadata["content"];
			if (end_bytes != tail_node_content["bounds"]["bytes"]["start"]) {
				for (auto bound : tail_node_content["bounds"].items()) {
					if (bound.key() == "bytes") {
						tail_bounds["bytes"] = {{"start", end_bytes},{"end", bound.value()["end"]}};
					} else {
						tail_bounds[bound.key()] = {{"start", bound.value()["start"]},{"end", bound.value()["end"]}};
					}
				}
			} else {
				tail_bounds = tail_node->metadata["content"]["bounds"];
			}
		} catch (std::out_of_range const &) {
			tail_node = &tail;
			tail_bounds = tail_node->metadata["content"]["bounds"];
		}

		// todo: when finding new lookup nodes, we just consolidate the old ones
		// when the depth is reasonable
		nlohmann::json lookup_nodes = nlohmann::json::array();
		//size_t depth = 0;
		nlohmann::json new_lookup_node;
		node preceding;
		lookup_nodes.clear();
		if (start_bytes > 0) { try {
			// ideally here we would get a node that has the span of interest aligned.  since that is where the user wants to put their data.
			preceding = this->get_node(tail, "bytes", start_bytes, {}, true, worker); // preceding 
			new_lookup_node = preceding.metadata["content"];
			new_lookup_node["identifiers"] = preceding.identifiers;
			new_lookup_node["depth"] = 0;
			// todo: the new lookup node has both bounds and spans.
			// this is both verbose and insufficient to calculate subranges of interest
			
			lookup_nodes = preceding.metadata["lookup"]; // everything in lookup nodes is accessible via preceding's identifiers
			lookup_nodes.emplace_back(new_lookup_node);
		} catch (std::out_of_range const &) { } }

		// 8: we have a new way of merging lookup nodes.  we merge all adjacent pairs with equal depth, repeatedly.
		// this means below algorithm should change to add new_lookup_node first, and then merge after adding.
		for (size_t index = 0; index + 1 < lookup_nodes.size();) {
			auto & current_node = lookup_nodes[index];
			auto & next_node = lookup_nodes[index + 1];
			if (current_node["depth"] == next_node["depth"]) {
				auto next_spans = next_node["spans"];
				for (auto & span : current_node["spans"].items()) {
					auto & current_span = span.value();
					auto & next_span = next_spans[span.key()];
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

					auto & current_end = current_span["end"];
					auto & next_end = next_span["end"];
					auto & next_start = next_span["start"];
					assert (current_end == next_start);
					if (current_end < next_end) { current_end = next_end; }
				}
				current_node["identifiers"] = preceding.identifiers;
				current_node["depth"] = (unsigned long long)current_node["depth"] + 1;
				lookup_nodes.erase(index + 1);
			} else {
				++ index;
			}
		}
		// 9: this is old implementation, below loop.  9: above loop is wip new implementation
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
		//  1. if !head_bounds.is_null(), then add a lookup reference for head
			// note: we can't merge this lookup node with previous because it is the only one with a link to its content.
		if (!head_bounds.is_null()) {
			lookup_nodes.emplace_back(nlohmann::json{
				{"identifiers", head_node->identifiers},
				{"spans", head_bounds},
				{"depth", 0} // now .... will this get merged if we append to tail after this?
						// when appending we assuming depth reduces forward, which is no longer true.
						// we probably want to reduce depth within as well as forward.
			});
		}

		//  2. if !tail_bounds.is_null(), then add a lookup reference for tail
		//  3. reference node hierarchies until real tail to complete reference to rest of doc

		auto content_identifiers = cryptography.digests({&data});
		nlohmann::json metadata_json = {
			{"sia-skynet-stream", "1.0.12"},
			{"content", {
				{"spans", spans},
				{"identifiers", content_identifiers},
			}},
			/* 10:
			{"flow", { 
				{"logical", lookup_nodes},
				{"creation", append_only_lookup_nodes_of_time_and_index}
			}},
			*/
			{"lookup", lookup_nodes}
		};
		if (! user_metadata.is_null()) {
			metadata_json["metadata"] = user_metadata;
		}
		std::string metadata_string = metadata_json.dump();
		//std::cerr << metadata_string << std::endl;

		sia::skynet::upload_data metadata_upload("metadata.json", std::vector<uint8_t>{metadata_string.begin(), metadata_string.end()}, "application/json");
		sia::skynet::upload_data content("content", data, "application/octet-stream");

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

		// if we want to support threading we'll likely need a lock around this whole function (not just the change to tail)
		// 	later: i've done that, but haven't integrated with old stuff to simplify
		tail.identifiers = metadata_identifiers;
		tail.metadata = metadata_json;
	}

	std::map<std::string,std::pair<double,double>> block_spans(std::string span, double offset, sia::portalpool::worker const * worker = 0)
	{
		std::lock_guard<std::mutex> lock(methodmtx);
		auto metadata = this->get_node(tail, span, offset, {}, false, worker).metadata;
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
		return lengths()[span];
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
				std::string identifier = identifiers.begin().value();
				if (!cache.count(identifier)) {
					cache[identifier] = node{identifiers, get_json(identifiers, nullptr, worker)};
				}
				return get_node(cache[identifier], span, offset, lookup_spans, get_preceding, worker);
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

	//nlohmann::json lookup_nodes(node & source, nlohmann::json & bounds)
	//{
	//	// to do this right, consider that source's content may be in the middle of its lookups.  so you want to put it in the right spot.
	//}

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
