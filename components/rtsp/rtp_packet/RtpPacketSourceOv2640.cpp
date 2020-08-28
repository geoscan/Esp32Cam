#include <esp_timer.h>
#include <utility>
#include "RtpPacketSourceOv2640.hpp"
#include "RtpPacketChunk.hpp"

using namespace std;

RtpPacketSourceOv2640::Timestamp RtpPacketSourceOv2640::currentTimeMs()
{
	return static_cast<Timestamp>(esp_timer_get_time() * 1000);
}

RtpPacketSourceOv2640::RtpPacketSourceOv2640(bool packForTcp, unsigned fps) :
    kFps(fps),
    mSequenceNumber{0},
	mTimestamp{1},
    mTcpTransport{packForTcp},
	mPrevMs{currentTimeMs()}
{
}

bool RtpPacketSourceOv2640::ready() const
{
	auto currMs = currentTimeMs();
	if (currMs - mPrevMs < kMinMsPerFrame) {
		return false;
	}

	return true;
}

void RtpPacketSourceOv2640::updateTimestamp()
{
	static constexpr Timestamp kRfc2435TimeUnitsHz = 90000; // const sample rate, https://tools.ietf.org/html/rfc2435#section-3

	auto     currMs    = currentTimeMs();
	auto     timeDelta = (currMs >= mPrevMs) ? currMs - mPrevMs : kMinMsPerFrame;

	mPrevMs = currMs;
	mTimestamp += timeDelta * (kRfc2435TimeUnitsHz / 1000);
}

Packets RtpPacketSourceOv2640::packets()
{
	if (!ready()) {
		return {};
	}

	auto     imagePtr = Ov2640::instance().jpeg();
	unsigned offset   = 0;
	BufPtr   data     = reinterpret_cast<BufPtr>(imagePtr->data().data());
	size_t   dataLen  = imagePtr->data().size();
	BufPtr   qtbl0;
	BufPtr   qtbl1;

//	if (!decodeJpegFile(&(imagePtr->data().data()),
//			imagePtr->data().size(),
//			&qtbl0, &qtbl1)) {
//		return {};
//	}

	if (!decodeJpegFile(&data, &dataLen, &qtbl0, &qtbl1)) {
		return {};
	}

	Packets packets;
	do {
		packets.emplace(move(nextPacket(data, dataLen, offset, qtbl0, qtbl1,
			imagePtr->frameBuffer()->width, imagePtr->frameBuffer()->height)));
	} while (offset != 0);

	updateTimestamp();
	return packets;
}

std::unique_ptr<RtpPacket> RtpPacketSourceOv2640::nextPacket(unsigned const char *jpeg, size_t jpegLen,
	unsigned &fragmentOffset, BufPtr quant0tbl, BufPtr quant1tbl, uint16_t mWidth, uint16_t mHeight)
{
#define KRtpHeaderSize 12           // size of the RTP header
#define KJpegHeaderSize 8           // size of the special JPEG payload header

#define MAX_FRAGMENT_SIZE 1100 // FIXME, pick more carefully
	int fragmentLen = MAX_FRAGMENT_SIZE;
	if(fragmentLen + fragmentOffset > jpegLen) // Shrink last fragment if needed
		fragmentLen = jpegLen - fragmentOffset;

	bool isLastFragment = (fragmentOffset + fragmentLen) == jpegLen;

	// Do we have custom quant tables? If so include them per RFC

	bool includeQuantTbl = quant0tbl && quant1tbl && fragmentOffset == 0;
	uint8_t q = includeQuantTbl ? 128 : 0x5e;

	static char RtpBuf[2048]; // Note: we assume single threaded, this large buf we keep off of the tiny stack
	size_t RtpPacketSize = fragmentLen + KRtpHeaderSize + KJpegHeaderSize + (includeQuantTbl ? (4 + 64 * 2) : 0);

	memset(RtpBuf,0x00,sizeof(RtpBuf));
	// Prepare the first 4 byte of the packet. This is the Rtp over Rtsp header in case of TCP based transport
	RtpBuf[0]  = '$';        // magic number
	RtpBuf[1]  = 0;          // number of multiplexed subchannel on RTPS connection - here the RTP channel
	RtpBuf[2]  = (RtpPacketSize & 0x0000FF00) >> 8;
	RtpBuf[3]  = (RtpPacketSize & 0x000000FF);
	// Prepare the 12 byte RTP header
	RtpBuf[4]  = 0x80;                               // RTP version
	RtpBuf[5]  = 0x1a | (isLastFragment ? 0x80 : 0x00);                               // JPEG payload (26) and marker bit
	RtpBuf[7]  = mSequenceNumber & 0x0FF;           // each packet is counted with a sequence counter
	RtpBuf[6]  = mSequenceNumber >> 8;
	RtpBuf[8]  = (mTimestamp & 0xFF000000) >> 24;   // each image gets a timestamp
	RtpBuf[9]  = (mTimestamp & 0x00FF0000) >> 16;
	RtpBuf[10] = (mTimestamp & 0x0000FF00) >> 8;
	RtpBuf[11] = (mTimestamp & 0x000000FF);
	RtpBuf[12] = 0x13;                               // 4 byte SSRC (sychronization source identifier)
	RtpBuf[13] = 0xf9;                               // we just an arbitrary number here to keep it simple
	RtpBuf[14] = 0x7e;
	RtpBuf[15] = 0x67;

	// Prepare the 8 byte payload JPEG header
	RtpBuf[16] = 0x00;                               // type specific
	RtpBuf[17] = (fragmentOffset & 0x00FF0000) >> 16;                               // 3 byte fragmentation offset for fragmented images
	RtpBuf[18] = (fragmentOffset & 0x0000FF00) >> 8;
	RtpBuf[19] = (fragmentOffset & 0x000000FF);

	/*    These sampling factors indicate that the chrominance components of
	   type 0 video is downsampled horizontally by 2 (often called 4:2:2)
	   while the chrominance components of type 1 video are downsampled both
	   horizontally and vertically by 2 (often called 4:2:0). */
	RtpBuf[20] = 0x00;                               // type (fixme might be wrong for camera data) https://tools.ietf.org/html/rfc2435
	RtpBuf[21] = q;                               // quality scale factor was 0x5e
	RtpBuf[22] = mWidth / 8;                           // width  / 8
	RtpBuf[23] = mHeight / 8;                           // height / 8

	int headerLen = 24; // Inlcuding jpeg header but not qant table header
	if(includeQuantTbl) { // we need a quant header - but only in first packet of the frame
		//printf("inserting quanttbl\n");
		RtpBuf[24] = 0; // MBZ
		RtpBuf[25] = 0; // 8 bit precision
		RtpBuf[26] = 0; // MSB of lentgh

		int numQantBytes = 64; // Two 64 byte tables
		RtpBuf[27] = 2 * numQantBytes; // LSB of length

		headerLen += 4;

		memcpy(RtpBuf + headerLen, quant0tbl, numQantBytes);
		headerLen += numQantBytes;

		memcpy(RtpBuf + headerLen, quant1tbl, numQantBytes);
		headerLen += numQantBytes;
	}
	// printf("Sending timestamp %d, seq %d, fragoff %d, fraglen %d, jpegLen %d\n", mTimestamp, mSequenceNumber, fragmentOffset, fragmentLen, jpegLen);

	// append the JPEG scan data to the RTP buffer
	memcpy(RtpBuf + headerLen,jpeg + fragmentOffset, fragmentLen);
	fragmentOffset += fragmentLen;

	mSequenceNumber++;                              // prepare the packet counter for the next packet

	if (mTcpTransport) {
		return unique_ptr<RtpPacket>(static_cast<RtpPacket *>(new RtpPacketChunk({RtpBuf, RtpPacketSize + 4})));
	} else {
		return unique_ptr<RtpPacket>(static_cast<RtpPacket *>(new RtpPacketChunk({RtpBuf + 4, RtpPacketSize})));
	}

	fragmentOffset = isLastFragment ? 0 : fragmentOffset;

//	// RTP marker bit must be set on last fragment
//	if (mTcpTransport) // RTP over RTSP - we send the buffer + 4 byte additional header
//		socketsend(m_Client,RtpBuf,RtpPacketSize + 4);
//	else                // UDP - we send just the buffer by skipping the 4 byte RTP over RTSP header
//		udpsocketsend(m_RtpSocket,&RtpBuf[4],RtpPacketSize, otherip, m_RtpClientPort);

//	return isLastFragment ? 0 : fragmentOffset;
}

// ------------------------- JPEG stuff ------------------------- //


// search for a particular JPEG marker, moves *start to just after that marker
// This function fixes up the provided start ptr to point to the
// actual JPEG stream data and returns the number of bytes skipped
// APP0 e0
// DQT db
// DQT db
// DHT c4
// DHT c4
// DHT c4
// DHT c4
// SOF0 c0 baseline (not progressive) 3 color 0x01 Y, 0x21 2h1v, 0x00 tbl0
// - 0x02 Cb, 0x11 1h1v, 0x01 tbl1 - 0x03 Cr, 0x11 1h1v, 0x01 tbl1
// therefore 4:2:2, with two separate quant tables (0 and 1)
// SOS da
// EOI d9 (no need to strip data after this RFC says client will discard)
bool RtpPacketSourceOv2640::findJpegHeader(BufPtr *start, uint32_t *len, uint8_t marker) {
    // per https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
    unsigned const char *bytes = *start;

    // kinda skanky, will break if unlucky and the headers inxlucde 0xffda
    // might fall off array if jpeg is invalid
    // FIXME - return false instead
    while(bytes - *start < *len) {
        uint8_t framing = *bytes++; // better be 0xff
        if(framing != 0xff) {
            printf("malformed jpeg, framing=%x\n", framing);
            return false;
        }
        uint8_t typecode = *bytes++;
        if(typecode == marker) {
            unsigned skipped = bytes - *start;
            //printf("found marker 0x%x, skipped %d\n", marker, skipped);

            *start = bytes;

            // shrink len for the bytes we just skipped
            *len -= skipped;

            return true;
        }
        else {
            // not the section we were looking for, skip the entire section
            switch(typecode) {
            case 0xd8:     // start of image
            {
                break;   // no data to skip
            }
            case 0xe0:   // app0
            case 0xdb:   // dqt
            case 0xc4:   // dht
            case 0xc0:   // sof0
            case 0xda:   // sos
            {
                // standard format section with 2 bytes for len.  skip that many bytes
                uint32_t len = bytes[0] * 256 + bytes[1];
                //printf("skipping section 0x%x, %d bytes\n", typecode, len);
                bytes += len;
                break;
            }
            default:
                printf("unexpected jpeg typecode 0x%x\n", typecode);
                break;
            }
        }
    }

    printf("failed to find jpeg marker 0x%x", marker);
    return false;
}

// the scan data uses byte stuffing to guarantee anything that starts with 0xff
// followed by something not zero, is a new section.  Look for that marker and return the ptr
// pointing there
void RtpPacketSourceOv2640::skipScanBytes(BufPtr *start) {
    BufPtr bytes = *start;

    while(true) { // FIXME, check against length
        while(*bytes++ != 0xff);
        if(*bytes++ != 0) {
            *start = bytes - 2; // back up to the 0xff marker we just found
            return;
        }
    }
}
void RtpPacketSourceOv2640::nextJpegBlock(BufPtr *bytes) {
    uint32_t len = (*bytes)[0] * 256 + (*bytes)[1];
    //printf("going to next jpeg block %d bytes\n", len);
    *bytes += len;
}

// When JPEG is stored as a file it is wrapped in a container
// This function fixes up the provided start ptr to point to the
// actual JPEG stream data and returns the number of bytes skipped
bool RtpPacketSourceOv2640::decodeJpegFile(BufPtr *start, uint32_t *len, BufPtr *qtable0, BufPtr *qtable1) {
    // per https://en.wikipedia.org/wiki/JPEG_File_Interchange_Format
    unsigned const char *bytes = *start;

    if(!findJpegHeader(&bytes, len, 0xd8)) // better at least look like a jpeg file
        return false; // FAILED!

    // Look for quant tables if they are present
    *qtable0 = NULL;
    *qtable1 = NULL;
    BufPtr quantstart = *start;
    uint32_t quantlen = *len;
    if(!findJpegHeader(&quantstart, &quantlen, 0xdb)) {
        printf("error can't find quant table 0\n");
    }
    else {
        // printf("found quant table %x\n", quantstart[2]);

        *qtable0 = quantstart + 3;     // 3 bytes of header skipped
        nextJpegBlock(&quantstart);
        if(!findJpegHeader(&quantstart, &quantlen, 0xdb)) {
            printf("error can't find quant table 1\n");
        }
        else {
            // printf("found quant table %x\n", quantstart[2]);
        }
        *qtable1 = quantstart + 3;
        nextJpegBlock(&quantstart);
    }

    if(!findJpegHeader(start, len, 0xda))
        return false; // FAILED!

    // Skip the header bytes of the SOS marker FIXME why doesn't this work?
    uint32_t soslen = (*start)[0] * 256 + (*start)[1];
    *start += soslen;
    *len -= soslen;

    // start scanning the data portion of the scan to find the end marker
    BufPtr endmarkerptr = *start;
    uint32_t endlen = *len;

    skipScanBytes(&endmarkerptr);
    if(!findJpegHeader(&endmarkerptr, &endlen, 0xd9))
        return false; // FAILED!

    // endlen must now be the # of bytes between the start of our scan and
    // the end marker, tell the caller to ignore bytes afterwards
    *len = endmarkerptr - *start;

    return true;
}

