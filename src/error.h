#ifndef _HTTP2_ERROR_H_
#define _HTTP2_ERROR_H_

namespace lhttp2{
	typedef enum _HTTP2_ERROR_CODE {
		HTTP2_ERROR_NO_ERROR = 0,
		HTTP2_ERROR_PROTOCOL_ERROR,
		HTTP2_ERROR_INTERNAL_ERROR,
		HTTP2_ERROR_FLOW_CONTROL_ERROR,
		HTTP2_ERROR_SETTINGS_TIMEOUT,
		HTTP2_ERROR_STREAM_CLOSED,
		HTTP2_ERROR_FRAME_SIZE_ERROR,
		HTTP2_ERROR_REFUSED_STREAM,
		HTTP2_ERROR_CANCEL,
		HTTP2_ERROR_COMPRESSION_ERROR,
		HTTP2_ERROR_CONNECT_ERROR,
		HTTP2_ERROR_ENHANCE_YOUR_CALM,
		HTTP2_ERROR_INADEQUATE_SECURITY,
		HTTP2_ERROR_HTTP_1_1_REQUIRED,
	} HTTP2_ERROR_CODE;
}

#endif