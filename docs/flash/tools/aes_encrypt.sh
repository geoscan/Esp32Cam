#!/bin/sh

command -v openssl >/dev/null 2>&1 || { echo >&2 "OpenSSL is required, but it's not installed."; exit 1; }

unset INPUT
unset OUTPUT
unset KEY

USAGE="USAGE: $0 -in <input-file> -out <output-file> -k <key in hex>"
ADVICE="Try '$0 -h' for more information"

while [ ! -z "$1" ]; do
        case $1 in
        -in)
                        INPUT="$2"
                        shift
                        ;;
        -out)
                        OUTPUT="$2"
                        shift
                        ;;
        -k)
                        KEY="$2"
                        shift
                        ;;
        -h|--help)
                        echo "$0 is a tool for generation of encrypted firmwares"
                        echo ""
                        echo "$USAGE"
                        echo ""
                        echo "Where:"
                        echo "  -h"
                        echo "          Show this message and exit"
                        echo "  -in <input-file>"
                        echo "          The full path to the firmware file to be encrypted"
                        echo "  -k <key in hex>"
                        echo "          Key in hex format for encryption (32 symbols)"
                        echo "          EXAMPLE: 00112233445566778899AABBCCDDEEFF"
                        echo "  -out <output-file>"
                        echo "          The full path to the encrypted firmware file to be created"
                        exit 0
                        ;;
        *)
                        break;
                        ;;
        esac
        shift
done

CONST_KEY_STR_LENGTH=32

if [ ! -f "$INPUT" ]; then
        echo "Input is not a file"
        echo $USAGE
        echo $ADVICE
        exit 1
fi

if [ -d $OUTPUT ]; then
        echo "Output is not a file"
        echo $USAGE
        echo $ADVICE
        exit 1
fi

if [ ${#KEY} != $CONST_KEY_STR_LENGTH ]; then
        echo "Wrong key lenght (should be $CONST_KEY_STR_LENGTH symbols)"
        exit 1
fi

if [ ! -f $OUTPUT ]; then
        touch $OUTPUT
fi

CONST_AES_IV="00000000000000000000000000000000"

RAND_BLOCK=$(cat /dev/urandom | tr -dc _A-Z-a-z-0-9 | head -c${1:-16})

TMPFILE=$(mktemp /tmp/aes_encrypt.XXXXXX)
exec 3>"$TMPFILE"
printf "%s" $RAND_BLOCK > $TMPFILE
cat $INPUT >> $TMPFILE

openssl aes-128-cbc -e -in $TMPFILE -out $OUTPUT -K $KEY -iv $CONST_AES_IV

rm "$TMPFILE"
exit 0
