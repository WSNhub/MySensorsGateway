#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <Libraries/Adafruit_SSD1306/Adafruit_SSD1306.h>
#include <globals.h>
#include <SDCard.h>

#include <AppSettings.h>

SdFileStream::SdFileStream(String fileName)
{
	Debug.printf("Opening file: %s\n", fileName.c_str());
	handle = SD.open(fileName);
        if (!handle)
	{
		Debug.printf("File wasn't found: %s\n", fileName.c_str());
		size = -1;
		pos = 0;
		return;
	}
	if (handle.isDirectory())
	{
		Debug.printf("File is a directory: %s\n", fileName.c_str());
		size = -1;
		pos = 0;
		return;
	}

	// Get size
	size = handle.size();

        // Set position
        pos = 0;

	Debug.printf("send file: %s (%d bytes)\n", fileName.c_str(), size);
}

SdFileStream::~SdFileStream()
{
	Debug.printf("CLEANUP: %s\n", handle.name());
        handle.close();
	//handle = 0;
	pos = 0;
}

uint16_t SdFileStream::readMemoryBlock(char* data, int bufSize)
{
	uint32_t len = min(bufSize, size - pos);
	len = min(1024, len);
        Debug.printf("READ: %s len %d\n", handle.name(), len);
	int available = handle.read(data, len);
	return available;
}

bool SdFileStream::seek(int len)
{
	if (len < 0) return false;

	bool result = fileSeek(handle, len, eSO_CurrentPos) >= 0;
	if (result) pos += len;
	return result;
}

bool SdFileStream::isFinished()
{
	return handle.available() < 1;
}

String SdFileStream::fileName()
{
	return handle.name();
}

bool SdFileStream::fileExist()
{
	return size != -1;
}

