#!/bin/bash

dir="./pki/intermediate"
certs="$dir/certs"			# Where the issued certs are kept
crl_dir="$dir/crl"			# Where the issued crl are kept
csr_dir="$dir/csr"			# Where the issued crl are kept
database="$dir/index.txt"	# database index file.
new_certs_dir="$dir/newcerts"	# default place for new certs.
private="$dir/private"
ecparams="./pki/ecparams"


filename="$1"
tmpfile=$(mktemp /tmp/signature.XXXXXX)
openssl dgst -sign $private/$1.key -out $tmpfile $2
singlen=$(cat $tmpfile | wc -c)
cat $2 > $2.gslic
printf "0: %.2x00" $singlen | xxd -r -g0 >> $2.gslic
cat $tmpfile >> $2.gslic
cat $certs/$1.der >> $2.gslic
rm $tmpfile
