#ifndef INCLUDE_SDCARD_H_
#define INCLUDE_SDCARD_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>
#include <SmingCore/DataSourceStream.h>
#include <SD/SD.h>

class SdFileStream : public IDataSourceStream
{
public:
	SdFileStream(String fileName);
	virtual ~SdFileStream();

	virtual StreamType getStreamType() { return eSST_File; }

	virtual uint16_t readMemoryBlock(char* data, int bufSize);
	virtual bool seek(int len);
	virtual bool isFinished();

	String fileName();
	bool fileExist();
	inline int getPos() { return pos; }

private:
	File handle;
	int pos;
	int size;
};

#endif //INCLUDE_SDCARD_H_
