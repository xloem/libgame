#!/usr/bin/env bash
EC_PRIVATE_KEY=00ffcfe1d3810dff2a69d676ee0ddf286d441f00595f20f8d89d84e50fabd060
BX=dependencies/prefix/Linux-$(uname -r)/bin/bx

EC_PUBLIC_KEY=$(echo $EC_PRIVATE_KEY | $BX ec-to-public)
ADDRESS=$($BX ec-to-address $EC_PUBLIC_KEY)
ADDRESSHEX=$($BX address-decode $ADDRESS | sed -ne 's/^\s*payload\s\(.*\)/\1/p')

echo Bitcoin Address: "$ADDRESS" "($ADDRESSHEX)"

RECEIVED=$(($($BX fetch-balance $ADDRESS | tee /dev/stderr | sed -ne 's/^\s*received\s\(.*\)/\1/p')))
SPENT=$(($($BX fetch-balance $ADDRESS | sed -ne 's/^\s*spent\s\(.*\)/\1/p')))
BALANCE=$((RECEIVED - SPENT))
echo Balance: $BALANCE

# 1. Get hash of previous transaction to spend from

UTXOHASH="$($BX fetch-utxo 1000 $ADDRESS | tee /dev/stderr | sed -ne 's/^\s*hash\s\(.*\)/\1/p' | head -n 1)"
UTXOINDEX="$($BX fetch-utxo 1000 $ADDRESS | tee /dev/stderr | sed -ne 's/^\s*index\s\(.*\)/\1/p' | head -n 1)"
UTXOVALUE="$($BX fetch-utxo 1000 $ADDRESS | tee /dev/stderr | sed -ne 's/^\s*value\s\(.*\)/\1/p' | head -n 1)"
echo Utxo: "$UTXOHASH":"$UTXOINDEX" "$UTXOVALUE" sat
#$BX fetch-tx "$HASH"


# 2. Encode the outputs scripts for the trasnaction
data='hello world'
datascript="return [$(echo -n "$data" | xxd -ps | tr -d '\n')]"
datascripthex="$($BX script-encode "$datascript")"
changescript="dup hash160 [""$ADDRESSHEX""] equalverify checksig"
changescripthex="$($BX script-encode "$changescript")"

# 3. Decide what transaction fee to bid to be included in the block chain and make the transaction output be the input amount - the fee

fee_per_kb=1000
fee_per_sigop=100
fee_extra=0

# generate a fake transaction to measure the fee
TX=$($BX tx-encode -i "$UTXOHASH":"$UTXOINDEX" -o "$datascripthex":0 -o "$changescripthex":"$((UTXOVALUE-1000))")
TX=$($BX input-set -i 0 "[$(xxd -ps -len 74 -cols 74 /dev/urandom)] [$EC_PUBLIC_KEY]" "$TX")
bytes=$(($(echo -n "$TX" | wc -c)/2))
sigops=4
fee=$((bytes*fee_per_kb/1000 + fee_per_sigop*sigops))
echo MAX SIZE: "$bytes" FEE: "$fee"

# 4. Generate the transaction including the fee
# bx tx-encode -i inputTransactionHash:transactionOutputIndex -o targetWalletAddress:amountToSendToWalletAddressAfterFeeSubtraction > fileToStoreResultingTransactionHexString
TX=$($BX tx-encode -i "$UTXOHASH":"$UTXOINDEX" -o "$datascripthex":0 -o "$changescripthex":"$((UTXOVALUE-fee))")


# 5. Look at the previous transaction and retrieve the output script for the utxo:
UTXOSCRIPT="$($BX fetch-tx "$UTXOHASH" | tr '\n' ' ' | sed -ne 's/.*address_hash\s'"$ADDRESSHEX"'\s\s*script "\([^"]*\)".*/\1/p')"

# 6. Make an endorsement of the input using the private key of the input owner and the decoded utxo script string
# bx input-sign privateKeyInHex oneOrMoreOutputScripts < transactionHexStringFile > inputEndorsementHexStringFile
INPUTSIG=$($BX input-sign -i 0 "$EC_PRIVATE_KEY" "$UTXOSCRIPT" "$TX")

# 7. Attach the endorsement to the input to sign the input
# bx input-set "[endorsement] [public key of endorser]" < fileToStoreResultingTransactionHexString > fileToStoreSignedTransactionHexString
SIGNEDTX=$($BX input-set -i 0 "[$INPUTSIG] [$EC_PUBLIC_KEY]" "$TX")
bytes=$(($(echo -n "$SIGNEDTX" | wc -c) / 2))

# 8. Optionally examine signed transaction
# bx tx-decode fileToStoreSignedTransactionHexString
TXHASH=$($BX tx-decode "$SIGNEDTX" | tee /dev/stderr | sed -ne 's/^\s*hash\s\(.*\)/\1/p')

# 9. Optionally validate the transaction against the blockchain
# bx validate-tx < fileToStoreSignedTransactionHexString
echo $fee fee $bytes bytes $sigops sigops "$fee_per_kb"/kb "$fee_per_sigop"/sigop
$BX validate-tx "$SIGNEDTX" || { echo '(failed validation)'; exit -1; }


# 10. Broadcast the signed transaction to the bitcoin network
# bx send-tx < fileToStoreSignedTransactionHexString
read "Press enter to broadcast '"$data"' as tx $TXHASH or ctrl-c to abort"
$BX send-tx-p2p "$SIGNEDTX"
$BX send-tx-node "$SIGNEDTX"

