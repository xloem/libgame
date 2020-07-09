#!/usr/bin/env bash
EC_PRIVATE_KEY=00ffcfe1d3810dff2a69d676ee0ddf286d441f00595f20f8d89d84e50fabd060
BX=dependencies/prefix/Linux-5.4.0-29-generic/bin/bx

EC_PUBLIC_KEY=$(echo $EC_PRIVATE_KEY | $BX ec-to-public)
ADDRESS=$($BX ec-to-address $EC_PUBLIC_KEY)

echo Bitcoin Address: "$ADDRESS"

BALANCE=$(($($BX fetch-balance $ADDRESS | tee /dev/stderr | sed -ne 's/^\s*received\s\(.*\)/\1/p')))
echo Balance: $BALANCE
HASH="$($BX fetch-utxo 1000 $ADDRESS | tee /dev/stderr | sed -ne 's/^\s*hash\s\(.*\)/\1/p' | head -n 1)"
echo Hash: "$HASH"
$BX fetch-tx "$HASH"
