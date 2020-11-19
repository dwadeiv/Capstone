/*
*********************************************************************************************************
*                                              Micrium OS
*                                               Network
*
*                             (c) Copyright 2004; Silicon Laboratories Inc.
*                                        https://www.micrium.com
*
*********************************************************************************************************
* Licensing:
*           YOUR USE OF THIS SOFTWARE IS SUBJECT TO THE TERMS OF A MICRIUM SOFTWARE LICENSE.
*   If you are not willing to accept the terms of an appropriate Micrium Software License, you must not
*   download or use this software for any reason.
*   Information about Micrium software licensing is available at https://www.micrium.com/licensing/
*   It is your obligation to select an appropriate license based on your intended use of the Micrium OS.
*   Unless you have executed a Micrium Commercial License, your use of the Micrium OS is limited to
*   evaluation, educational or personal non-commercial uses. The Micrium OS may not be redistributed or
*   disclosed to any third party without the written consent of Silicon Laboratories Inc.
*********************************************************************************************************
* Documentation:
*   You can find user manuals, API references, release notes and more at: https://doc.micrium.com
*********************************************************************************************************
* Technical Support:
*   Support is available for commercially licensed users of Micrium's software. For additional
*   information on support, you can contact info@micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                                HTTP
*
* File : http.h
*********************************************************************************************************
* Note(s) : (1) The http.c/h files gather defines, data types, structures and functions that are
*               common to HTTP client and server.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*********************************************************************************************************
*                                               MODULE
*********************************************************************************************************
*********************************************************************************************************
*/

#ifndef  _HTTP_H_
#define  _HTTP_H_


/*
*********************************************************************************************************
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         METHODS ENUMARATION
*********************************************************************************************************
*/

typedef enum http_method {
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_TRACE,
    HTTP_METHOD_CONNECT,
    HTTP_METHOD_UNKNOWN
} HTTP_METHOD;


/*
*********************************************************************************************************
*                                      STATUS CODES ENUMARATION
*********************************************************************************************************
*/

typedef enum http_status_code {
    HTTP_STATUS_OK,
    HTTP_STATUS_CREATED,
    HTTP_STATUS_ACCEPTED,
    HTTP_STATUS_NO_CONTENT,
    HTTP_STATUS_RESET_CONTENT,
    HTTP_STATUS_MOVED_PERMANENTLY,
    HTTP_STATUS_FOUND,
    HTTP_STATUS_SEE_OTHER,
    HTTP_STATUS_NOT_MODIFIED,
    HTTP_STATUS_USE_PROXY,
    HTTP_STATUS_TEMPORARY_REDIRECT,
    HTTP_STATUS_BAD_REQUEST,
    HTTP_STATUS_UNAUTHORIZED,
    HTTP_STATUS_FORBIDDEN,
    HTTP_STATUS_NOT_FOUND,
    HTTP_STATUS_METHOD_NOT_ALLOWED,
    HTTP_STATUS_NOT_ACCEPTABLE,             /* With the Accept Req Hdr */
    HTTP_STATUS_REQUEST_TIMEOUT,
    HTTP_STATUS_CONFLICT,
    HTTP_STATUS_GONE,
    HTTP_STATUS_LENGTH_REQUIRED,
    HTTP_STATUS_PRECONDITION_FAILED,
    HTTP_STATUS_REQUEST_ENTITY_TOO_LARGE,
    HTTP_STATUS_REQUEST_URI_TOO_LONG,
    HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
    HTTP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE,
    HTTP_STATUS_EXPECTATION_FAILED,
    HTTP_STATUS_INTERNAL_SERVER_ERR,
    HTTP_STATUS_NOT_IMPLEMENTED,
    HTTP_STATUS_SERVICE_UNAVAILABLE,
    HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED,
    HTTP_STATUS_SWITCHING_PROTOCOLS,
    HTTP_STATUS_UNKOWN
} HTTP_STATUS_CODE;


/*
*********************************************************************************************************
*                                     HTTP HEADER ENUMARATION
*********************************************************************************************************
*/

typedef  enum http_hdr_field {
    HTTP_HDR_FIELD_UNKNOWN,
    HTTP_HDR_FIELD_CONTENT_TYPE,
    HTTP_HDR_FIELD_CONTENT_LEN,
    HTTP_HDR_FIELD_CONTENT_DISPOSITION,
    HTTP_HDR_FIELD_HOST,
    HTTP_HDR_FIELD_LOCATION,
    HTTP_HDR_FIELD_CONN,
    HTTP_HDR_FIELD_TRANSFER_ENCODING,
    HTTP_HDR_FIELD_ACCEPT,
    HTTP_HDR_FIELD_ACCEPT_CHARSET,
    HTTP_HDR_FIELD_ACCEPT_ENCODING,
    HTTP_HDR_FIELD_ACCEPT_LANGUAGE,
    HTTP_HDR_FIELD_ACCEPT_RANGES,
    HTTP_HDR_FIELD_AGE,
    HTTP_HDR_FIELD_ALLOW,
    HTTP_HDR_FIELD_AUTHORIZATION,
    HTTP_HDR_FIELD_CONTENT_ENCODING,
    HTTP_HDR_FIELD_CONTENT_LANGUAGE,
    HTTP_HDR_FIELD_CONTENT_LOCATION,
    HTTP_HDR_FIELD_CONTENT_MD5,
    HTTP_HDR_FIELD_CONTENT_RANGE,
    HTTP_HDR_FIELD_COOKIE,
    HTTP_HDR_FIELD_COOKIE2,
    HTTP_HDR_FIELD_DATE,
    HTTP_HDR_FIELD_ETAG,
    HTTP_HDR_FIELD_EXPECT,
    HTTP_HDR_FIELD_EXPIRES,
    HTTP_HDR_FIELD_FROM,
    HTTP_HDR_FIELD_IF_MODIFIED_SINCE,
    HTTP_HDR_FIELD_IF_MATCH,
    HTTP_HDR_FIELD_IF_NONE_MATCH,
    HTTP_HDR_FIELD_IF_RANGE,
    HTTP_HDR_FIELD_IF_UNMODIFIED_SINCE,
    HTTP_HDR_FIELD_LAST_MODIFIED,
    HTTP_HDR_FIELD_RANGE,
    HTTP_HDR_FIELD_REFERER,
    HTTP_HDR_FIELD_RETRY_AFTER,
    HTTP_HDR_FIELD_SERVER,
    HTTP_HDR_FIELD_SET_COOKIE,
    HTTP_HDR_FIELD_SET_COOKIE2,
    HTTP_HDR_FIELD_TE,
    HTTP_HDR_FIELD_TRAILER,
    HTTP_HDR_FIELD_UPGRADE,
    HTTP_HDR_FIELD_USER_AGENT,
    HTTP_HDR_FIELD_VARY,
    HTTP_HDR_FIELD_VIA,
    HTTP_HDR_FIELD_WARNING,
    HTTP_HDR_FIELD_WWW_AUTHENTICATE,
    HTTP_HDR_FIELD_WEBSOCKET_KEY,
    HTTP_HDR_FIELD_WEBSOCKET_ACCEPT,
    HTTP_HDR_FIELD_WEBSOCKET_VERSION,
    HTTP_HDR_FIELD_WEBSOCKET_PROTOCOL,
    HTTP_HDR_FIELD_WEBSOCKET_EXTENSIONS
} HTTP_HDR_FIELD;


/*
*********************************************************************************************************
*                                     HTTPS VERSIONS ENUMARATION
*********************************************************************************************************
*/

typedef  enum  http_protocol_ver {
    HTTP_PROTOCOL_VER_0_9,
    HTTP_PROTOCOL_VER_1_0,
    HTTP_PROTOCOL_VER_1_1,
    HTTP_PROTOCOL_VER_UNKNOWN
} HTTP_PROTOCOL_VER;


/*
*********************************************************************************************************
*                                      CONTENT TYPES ENUMARATION
*********************************************************************************************************
*/

typedef enum http_content_type {
    HTTP_CONTENT_TYPE_UNKNOWN,
    HTTP_CONTENT_TYPE_NONE,
    HTTP_CONTENT_TYPE_HTML,
    HTTP_CONTENT_TYPE_OCTET_STREAM,
    HTTP_CONTENT_TYPE_PDF,
    HTTP_CONTENT_TYPE_ZIP,
    HTTP_CONTENT_TYPE_GIF,
    HTTP_CONTENT_TYPE_JPEG,
    HTTP_CONTENT_TYPE_PNG,
    HTTP_CONTENT_TYPE_JS,
    HTTP_CONTENT_TYPE_PLAIN,
    HTTP_CONTENT_TYPE_CSS,
    HTTP_CONTENT_TYPE_JSON,
    HTTP_CONTENT_TYPE_APP_FORM,
    HTTP_CONTENT_TYPE_MULTIPART_FORM,
    HTTP_CONTENT_TYPE_WEBSOCK_TXT_DATA,
    HTTP_CONTENT_TYPE_WEBSOCK_BIN_DATA
} HTTP_CONTENT_TYPE;


/*
*********************************************************************************************************
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*********************************************************************************************************
*/

#endif  /* _HTTP_H_ */
