#!/bin/bash
BINARY_NAME=$1
MAKE_FIPS_BINARY()
{
	openssl dgst -sha256 -hmac 12345678 -binary -out \
		$BINARY_NAME.hmac $BINARY_NAME
	cat $BINARY_NAME $BINARY_NAME.hmac > $BINARY_NAME.digest
	cp -f $BINARY_NAME.digest $BINARY_NAME
	rm -f $BINARY_NAME.digest $BINARY_NAME.hmac
}

echo "Make kernel fips binary.."
MAKE_FIPS_BINARY
echo "Done."


