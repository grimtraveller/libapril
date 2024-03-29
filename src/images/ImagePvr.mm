/*

    File: PVRTexture.m
Abstract: The PVRTexture class is responsible for loading .pvr files.
 Version: 1.2

Disclaimer: IMPORTANT:  This Apple software is supplied to you by Apple
Inc. ("Apple") in consideration of your agreement to the following
terms, and your use, installation, modification or redistribution of
this Apple software constitutes acceptance of these terms.  If you do
not agree with these terms, please do not use, install, modify or
redistribute this Apple software.

In consideration of your agreement to abide by the following terms, and
subject to these terms, Apple grants you a personal, non-exclusive
license, under Apple's copyrights in this original Apple software (the
"Apple Software"), to use, reproduce, modify and redistribute the Apple
Software, with or without modifications, in source and/or binary forms;
provided that if you redistribute the Apple Software in its entirety and
without modifications, you must retain this notice and the following
text and disclaimers in all such redistributions of the Apple Software.
Neither the name, trademarks, service marks or logos of Apple Inc. may
be used to endorse or promote products derived from the Apple Software
without specific prior written permission from Apple.  Except as
expressly stated in this notice, no other rights or licenses, express or
implied, are granted by Apple herein, including but not limited to any
patent rights that may be infringed by your derivative works or by other
works in which the Apple Software may be incorporated.

The Apple Software is provided by Apple on an "AS IS" basis.  APPLE
MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND
OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.

IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
MODIFICATION AND/OR DISTRIBUTION OF THE APPLE SOFTWARE, HOWEVER CAUSED
AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Copyright (C) 2010 Apple Inc. All Rights Reserved.


*/

#include "Image.h"
#include <hltypes/hresource.h>
#include <hltypes/hdir.h>
#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>

// fix for NSURL not working on iPad Simulator
#ifndef __GNUC__
#define __asm__ asm
#endif

__asm__(".weak_reference _OBJC_CLASS_$_NSURL");
// end iPad Simulator fix

#define PVR_TEXTURE_FLAG_TYPE_MASK	0xff

static char gPVRTexIdentifier[5] = "PVR!";

enum
{
	kPVRTextureFlagTypePVRTC_2 = 24,
	kPVRTextureFlagTypePVRTC_4
};

typedef struct _PVRTexHeader
{
	uint32_t headerLength;
	uint32_t height;
	uint32_t width;
	uint32_t numMipmaps;
	uint32_t flags;
	uint32_t dataLength;
	uint32_t bpp;
	uint32_t bitmaskRed;
	uint32_t bitmaskGreen;
	uint32_t bitmaskBlue;
	uint32_t bitmaskAlpha;
	uint32_t pvrTag;
	uint32_t numSurfs;
} PVRTexHeader;

@interface PVRTexture : NSObject
{
	NSMutableArray *_imageData;

	GLuint _name;
	uint32_t _width, _height;
	GLenum _internalFormat;
	BOOL _hasAlpha;
}

- (id)initWithMemoryBuffer:(uint8_t*) data dataSize:(int) dataSize;
- (id)initWithContentsOfFile:(NSString *)path;
- (id)initWithContentsOfURL:(NSURL *)url;
+ (id)pvrTextureWithContentsOfFile:(NSString *)path;
+ (id)pvrTextureWithContentsOfURL:(NSURL *)url;
+ (id)pvrTextureWithMemoryBuffer:(uint8_t*) data dataSize:(int) dataSize;

@property (readonly) GLuint name;
@property (readonly) uint32_t width;
@property (readonly) uint32_t height;
@property (readonly) GLenum internalFormat;
@property (readonly) BOOL hasAlpha;

@property (readonly) NSMutableArray *imageData;

@end





@implementation PVRTexture

@synthesize name = _name;
@synthesize width = _width;
@synthesize height = _height;
@synthesize internalFormat = _internalFormat;
@synthesize hasAlpha = _hasAlpha;
@synthesize imageData = _imageData;

- (BOOL)unpackPVRData:(uint8_t*) data dataSize:(int) dataSize
{
	BOOL success = FALSE;
	PVRTexHeader *header = NULL;
	uint32_t flags, pvrTag;
	uint32_t dataLength = 0, dataOffset = 0;
	uint32_t blockSize = 0, widthBlocks = 0, heightBlocks = 0;
	uint32_t width = 0, height = 0, bpp = 4;
	uint8_t *bytes = NULL;
	uint32_t formatFlags;

	header = (PVRTexHeader*) data;

	pvrTag = CFSwapInt32LittleToHost(header->pvrTag);

	if (gPVRTexIdentifier[0] != ((pvrTag >>  0) & 0xff) ||
		gPVRTexIdentifier[1] != ((pvrTag >>  8) & 0xff) ||
		gPVRTexIdentifier[2] != ((pvrTag >> 16) & 0xff) ||
		gPVRTexIdentifier[3] != ((pvrTag >> 24) & 0xff))
	{
		return FALSE;
	}

	flags = CFSwapInt32LittleToHost(header->flags);
	formatFlags = flags & PVR_TEXTURE_FLAG_TYPE_MASK;

	if (formatFlags == kPVRTextureFlagTypePVRTC_4 || formatFlags == kPVRTextureFlagTypePVRTC_2)
	{
		[_imageData removeAllObjects];

		if (formatFlags == kPVRTextureFlagTypePVRTC_4)
			_internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		else if (formatFlags == kPVRTextureFlagTypePVRTC_2)
			_internalFormat = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;

		_width = width = CFSwapInt32LittleToHost(header->width);
		_height = height = CFSwapInt32LittleToHost(header->height);

		if (CFSwapInt32LittleToHost(header->bitmaskAlpha))
			_hasAlpha = TRUE;
		else
			_hasAlpha = FALSE;

		dataLength = CFSwapInt32LittleToHost(header->dataLength);

		bytes = ((uint8_t *)data) + sizeof(PVRTexHeader);

		// Calculate the data size for each texture level and respect the minimum number of blocks
		while (dataOffset < dataLength)
		{
			if (formatFlags == kPVRTextureFlagTypePVRTC_4)
			{
				blockSize = 4 * 4; // Pixel by pixel block size for 4bpp
				widthBlocks = width / 4;
				heightBlocks = height / 4;
				bpp = 4;
			}
			else
			{
				blockSize = 8 * 4; // Pixel by pixel block size for 2bpp
				widthBlocks = width / 8;
				heightBlocks = height / 4;
				bpp = 2;
			}

			// Clamp to minimum number of blocks
			if (widthBlocks < 2)
				widthBlocks = 2;
			if (heightBlocks < 2)
				heightBlocks = 2;

			dataSize = widthBlocks * heightBlocks * ((blockSize  * bpp) / 8);

			[_imageData addObject:[NSData dataWithBytes:bytes+dataOffset length:dataSize]];

			dataOffset += dataSize;

			width = MAX(width >> 1, 1);
			height = MAX(height >> 1, 1);
		}

		success = TRUE;
	}

	return success;
}

- (BOOL)unpackPVRNSData:(NSData *)data
{
	return [self unpackPVRData:(uint8_t*) [data bytes] dataSize:(int)[data length]];
}


- (BOOL)createGLTexture
{
	int width = _width;
	int height = _height;
	NSData *data;
	GLenum err;
	
	if ([_imageData count] > 0)
	{
		if (_name != 0)
			glDeleteTextures(1, &_name);
		
		glGenTextures(1, &_name);
		glBindTexture(GL_TEXTURE_2D, _name);
	}
	
	if ([_imageData count] > 1)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	else
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	for (int i=0; i < [_imageData count]; i++)
	{
		data = [_imageData objectAtIndex:i];
		glCompressedTexImage2D(GL_TEXTURE_2D, i, _internalFormat, width, height, 0, (int)[data length], [data bytes]);
		
		err = glGetError();
		if (err != GL_NO_ERROR)
		{
			NSLog(@"Error uploading compressed texture level: %d. glError: 0x%04X", i, err);
			return FALSE;
		}
		
		width = MAX(width >> 1, 1);
		height = MAX(height >> 1, 1);
	}
	
	[_imageData removeAllObjects];
	
	return TRUE;
}

- (id)initWithMemoryBuffer:(uint8_t*) data dataSize:(int) dataSize
{
	if (self = [super init])
	{
		_imageData = [[NSMutableArray alloc] initWithCapacity:10];
		_name = 0;
		_width = _height = 0;
		_internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		_hasAlpha = FALSE;

		if (!data || ![self unpackPVRData:data dataSize:dataSize])
		{
			[self release];
			self = nil;
		}
	}

	return self;
}


- (id)initWithContentsOfFile:(NSString *)path
{
	if (self = [super init])
	{
		NSData *data = [NSData dataWithContentsOfFile:path];

		_imageData = [[NSMutableArray alloc] initWithCapacity:10];

		_name = 0;
		_width = _height = 0;
		_internalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		_hasAlpha = FALSE;

		if (!data || ![self unpackPVRNSData:data])
		{
			[self release];
			self = nil;
		}
	}

	return self;
}

- (id)initWithContentsOfURL:(NSURL *)url
{
	if (![url isFileURL])
	{
		[self release];
		return nil;
	}
	
	return [self initWithContentsOfFile:[url path]];
}

+ (id)pvrTextureWithMemoryBuffer:(uint8_t*) data dataSize:(int) dataSize
{
	return [[[self alloc] initWithMemoryBuffer:data dataSize:dataSize] autorelease];
}

+ (id)pvrTextureWithContentsOfFile:(NSString *)path
{
	return [[[self alloc] initWithContentsOfFile:path] autorelease];
}


+ (id)pvrTextureWithContentsOfURL:(NSURL *)url
{
	if (![url isFileURL])
		return nil;
	
	return [PVRTexture pvrTextureWithContentsOfFile:[url path]];
}


- (void)dealloc
{
	[_imageData release];
	
	if (_name != 0)
		glDeleteTextures(1, &_name);
	
	[super dealloc];
}

@end

namespace april
{
	NSURL* _getFileURLAsResource(chstr filename)
	{
		NSString* resources = [[NSBundle mainBundle] resourcePath];
		NSString* file;
		if (filename.startsWith("/"))
		{
			file = [NSString stringWithUTF8String:filename.cStr()];
		}
		else
		{
			file = [resources stringByAppendingPathComponent:[NSString stringWithUTF8String:filename.cStr()]];
		}
		NSURL * url = [NSURL URLWithString:[@"file://" stringByAppendingString:[file stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] ]];
		return url;
	}

	Image* _tryLoadingPVR(unsigned char* data, int dataLen)
	{
		NSAutoreleasePool* arp = [[NSAutoreleasePool alloc] init];
		PVRTexture* pvrtex = [PVRTexture pvrTextureWithMemoryBuffer:data dataSize:dataLen];
		if(!pvrtex)
		{
			return NULL;
		}

		NSData* imageData = [pvrtex.imageData objectAtIndex:0];
		Image* image = Image::create(pvrtex.width, pvrtex.height, NULL, Image::FORMAT_INVALID);
		image->data = new unsigned char[imageData.length];
		memcpy(image->data, imageData.bytes, imageData.length);
		image->format = Image::FORMAT_PALETTE;
		image->internalFormat = pvrtex.internalFormat;
		image->compressedSize = (int)imageData.length;
		[arp release];
		return image;
	}

	Image* _tryLoadingPVR(hsbase& stream)
	{
		PVRTexHeader header;
		int headerSize = sizeof(header);
		stream.readRaw(&header, headerSize);

		unsigned char* buffer = new unsigned char[headerSize + header.dataLength];
		memcpy(buffer, &header, headerSize);
		stream.readRaw(buffer + headerSize, header.dataLength);
		Image* image = _tryLoadingPVR(buffer, headerSize + header.dataLength);
		delete [] buffer;
		return image;
	}

	Image* Image::_loadPvr(hsbase& stream)
	{
		return _tryLoadingPVR(stream);
	}

	Image* _tryLoadingPVRMetaData(unsigned char* data, int dataLen)
	{
		NSAutoreleasePool* arp = [[NSAutoreleasePool alloc] init];
		PVRTexture* pvrtex = [PVRTexture pvrTextureWithMemoryBuffer:data dataSize:dataLen];
		if(!pvrtex)
		{
			return NULL;
		}

		NSData* imageData = [pvrtex.imageData objectAtIndex:0];
		Image* image = Image::create(pvrtex.width, pvrtex.height, NULL, Image::FORMAT_INVALID);;
		image->w = pvrtex.width;
		image->h = pvrtex.height;
		image->data = NULL;
		image->format = Image::FORMAT_PALETTE;
		image->internalFormat = pvrtex.internalFormat;
		image->compressedSize = (int)imageData.length;
		[arp release];
		return image;
	}

	Image* _tryLoadingPVRMetaData(hsbase& stream)
	{
		PVRTexHeader header;
		int headerSize = sizeof(header);
		stream.readRaw(&header, headerSize);

		unsigned char* buffer = new unsigned char[headerSize + header.dataLength];
		memcpy(buffer, &header, headerSize);
		stream.readRaw(buffer + headerSize, header.dataLength);
		Image* image = _tryLoadingPVRMetaData(buffer, headerSize + header.dataLength);
		delete [] buffer;
		return image;
	}

	Image* Image::_readMetaDataPvr(hsbase& stream)
	{
		return _tryLoadingPVRMetaData(stream);
	}
	
}
