/*
* 
* Copyright (c) 2015, Wieden+Kennedy, 
* Stephen Schieberl, Michael Latzoni
* All rights reserved.
* 
* Redistribution and use in source and binary forms, with or 
* without modification, are permitted provided that the following 
* conditions are met:
* 
* Redistributions of source code must retain the above copyright 
* notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright 
* notice, this list of conditions and the following disclaimer in 
* the documentation and/or other materials provided with the 
* distribution.
* 
* Neither the name of the Ban the Rewind nor the names of its 
* contributors may be used to endorse or promote products 
* derived from this software without specific prior written 
* permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
* COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
*/

#include "SessionInterface.h"

using namespace ci;
using namespace std;

string SessionInterface::bufferToString( const BufferRef& buffer )
{
	string s( static_cast<const char*>( buffer->getData() ) );
	if ( s.length() > buffer->getSize() ) {
		s.resize( buffer->getSize() );
	}
	return s;
}

BufferRef SessionInterface::stringToBuffer( string& value )
{
	return Buffer::create( &value[ 0 ], value.size() );
}

SessionInterface::SessionInterface( asio::io_service& io )
: DispatcherInterface( io ), mBufferSize( 0 ), mReadCompleteEventHandler( nullptr ), 
mReadEventHandler( nullptr ), mWriteEventHandler( nullptr )
{
}

SessionInterface::~SessionInterface()
{
	mReadCompleteEventHandler	= nullptr;
	mReadEventHandler			= nullptr;
	mWriteEventHandler			= nullptr;
	mRequest.consume( mRequest.size() );
	mResponse.consume( mResponse.size() );
}

void SessionInterface::onRead( const asio::error_code& err, size_t bytesTransferred )
{
	if ( err ) {
		if ( err == asio::error::eof ) {
			if ( mReadCompleteEventHandler != nullptr ) {
				mReadCompleteEventHandler();
			}
		} else {
			if ( mErrorEventHandler != nullptr ) {
				mErrorEventHandler( err.message(), bytesTransferred );
			}
		}
	} else {
		if ( mReadEventHandler != nullptr ) {
			char* data = new char[ bytesTransferred + 1 ]();
			data[ bytesTransferred ] = 0;
			mResponse.commit( bytesTransferred );
			istream stream( &mResponse );
			stream.read( data, bytesTransferred );
			mReadEventHandler( Buffer::create( data, bytesTransferred ) );
			delete [] data;
		}
		if ( mReadCompleteEventHandler != nullptr && 
			mBufferSize > 0 && bytesTransferred < mBufferSize ) {
			mReadCompleteEventHandler();
		}
	}
	mResponse.consume( mResponse.size() );
}

void SessionInterface::onWrite( const asio::error_code& err, size_t bytesTransferred )
{
	if ( err ) {
		if ( mErrorEventHandler != nullptr ) {
			mErrorEventHandler( err.message(), bytesTransferred );
		}
	} else {
		if ( mWriteEventHandler != nullptr ) {
			mWriteEventHandler( bytesTransferred );
		}
	}
}

void SessionInterface::connectReadEventHandler( const std::function<void( ci::BufferRef )>& eventHandler )
{
	mReadEventHandler = eventHandler;
}

void SessionInterface::connectReadCompleteEventHandler( const std::function<void ()>& eventHandler )
{
	mReadCompleteEventHandler = eventHandler;
}

void SessionInterface::connectWriteEventHandler( const std::function<void( size_t )>& eventHandler )
{
	mWriteEventHandler = eventHandler;
}