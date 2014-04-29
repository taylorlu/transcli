#ifndef __MP4_CHUNK_H
#define __MP4_CHUNK_H

class Mp4Chunk
{
public:
	Mp4Chunk(void);
	~Mp4Chunk(void);

public:
	unsigned long long mOffset;
	unsigned int mIndex;
	unsigned int mSamples;
};

#endif
