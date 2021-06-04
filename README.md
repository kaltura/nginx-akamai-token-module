# Secure token module for Nginx [![Build Status](https://travis-ci.org/kaltura/nginx-secure-token-module.svg?branch=master)](https://travis-ci.org/kaltura/nginx-secure-token-module)

Generates CDN tokens, either as a cookie or as a query string parameter (m3u8,mpd,f4m only).
Currently supports Akamai v2 tokens, and Amazon CloudFront tokens.
In addition, the module supports the encryption of URIs with a configured key.

## Build

To link statically against Nginx, cd to the Nginx source directory and execute:

    ./configure --add-module=/path/to/nginx-secure-token-module

To compile as a dynamic module (Nginx 1.9.11+), use:
  
	./configure --add-dynamic-module=/path/to/nginx-secure-token-module

In this case, the `load_module` directive should be used in nginx.conf to load the module.

Requires OpenSSL.

## Configuration

### Generic token parameters

#### `secure_token`
* **syntax**: `secure_token value`
* **default**: `none`
* **context**: `http`, `server`, `location`

Sets the value of the token that should be embedded in the manifest/returned as a cookie.
The parameter value can contain variables, and often points to variables set by this module
(using `secure_token_akamai` / `secure_token_cloudfront` blocks)

#### `secure_token_avoid_cookies`
* **syntax**: `secure_token_avoid_cookies on/off`
* **default**: `on`
* **context**: `http`, `server`, `location`

When enabled the module prefers to use a query string token instead of a cookie token.
A query string token is currently supported only for the following mime types (other mime types return a cookie token):
* `application/vnd.apple.mpegurl`
* `application/dash+xml`
* `video/f4m`

#### `secure_token_types`
* **syntax**: `secure_token_types mime_type ...`
* **default**: `none`
* **context**: `http`, `server`, `location`

Defines a set of mime types that should return a token

#### `secure_token_uri_filename_prefix`
* **syntax**: `secure_token_uri_filename_prefix prefix`
* **default**: `none`
* **context**: `http`, `server`, `location`

Defines a set of prefixes that will be matched against the URI file name, only URIs whose file name
starts with one of the defined prefixes will return a token

#### `secure_token_expires_time`
* **syntax**: `secure_token_expires_time time`
* **default**: `none`
* **context**: `http`, `server`, `location`

Sets the expiration time of responses that are not tokenized 
(determines the values of the Cache-Control and Expires HTTP headers)

#### `secure_token_cookie_token_expires_time`
* **syntax**: `secure_token_cookie_token_expires_time time`
* **default**: `none`
* **context**: `http`, `server`, `location`

Sets the expiration time of responses that are tokenized with a cookie token 
(determines the values of the Cache-Control and Expires HTTP headers)

#### `secure_token_query_token_expires_time`
* **syntax**: `secure_token_query_token_expires_time time`
* **default**: `none`
* **context**: `http`, `server`, `location`

Sets the expiration time of responses that are tokenized with a query string token 
(determines the values of the Cache-Control and Expires HTTP headers)

#### `secure_token_cache_scope`
* **syntax**: `secure_token_cache_scope scope`
* **default**: `public`
* **context**: `http`, `server`, `location`

Sets the cache scope (public/private) of responses that are not tokenized

#### `secure_token_token_cache_scope`
* **syntax**: `secure_token_token_cache_scope scope`
* **default**: `private`
* **context**: `http`, `server`, `location`

Sets the cache scope (public/private) of responses that are tokenized (query / cookie)

#### `secure_token_last_modified`
* **syntax**: `secure_token_last_modified time`
* **default**: `Sun, 19 Nov 2000 08:52:00 GMT`
* **context**: `http`, `server`, `location`

Sets the value of the last-modified header of responses that are not tokenized.
An empty string leaves the value of last-modified unaltered, while the string "now" sets the header to the server current time.

#### `secure_token_token_last_modified`
* **syntax**: `secure_token_token_last_modified time`
* **default**: `now`
* **context**: `http`, `server`, `location`

Sets the value of the last-modified header of responses that are tokenized (query / cookie)
An empty string leaves the value of last-modified unaltered, while the string "now" sets the header to the server current time.

#### `secure_token_content_type_m3u8`
* **syntax**: `secure_token_content_type_m3u8 type`
* **default**: `application/vnd.apple.mpegurl`
* **context**: `http`, `server`, `location`

Sets the content type that should be parsed as m3u8 for token insertion

#### `secure_token_content_type_mpd`
* **syntax**: `secure_token_content_type_mpd type`
* **default**: `application/dash+xml`
* **context**: `http`, `server`, `location`

Sets the content type that should be parsed as mpd for token insertion

#### `secure_token_content_type_f4m`
* **syntax**: `secure_token_content_type_f4m type`
* **default**: `video/f4m`
* **context**: `http`, `server`, `location`

Sets the content type that should be parsed as f4m for token insertion

### Akamai token parameters

#### `secure_token_akamai`
* **syntax**: `secure_token_akamai $variable { ... }`
* **context**: `http`

Creates a new variable whose value is an Akamai token, created according to the 
parameters specified within the block.

The block supports the following parameters:

#### key
* **syntax**: `key key_hex`
* **default**: `N/A (mandatory)`

Sets the secret key.

#### `param_name`
* **syntax**: `param_name name`
* **default**: `__hdnea__`

Sets the token parameter name (either the name of the cookie or the query string parameter)

#### `acl`
* **syntax**: `acl acl`
* **default**: `$secure_token_baseuri_comma`

Sets the signed part of the URL (ACL). The parameter value can contain variables.

#### `start`
* **syntax**: `start time`
* **default**: `0`

Sets the start time of the token (see `Time format` below)

#### `end`
* **syntax**: `end time`
* **default**: `86400`

Sets the end time of the token (see `Time format` below)

#### `ip_address`
* **syntax**: `ip_address address`
* **default**: `none`

Sets the IP address that should be embedded in the token.
The parameter value can contain variables, e.g. `$remote_addr`.

### CloudFront token parameters

#### `secure_token_cloudfront`
* **syntax**: `secure_token_cloudfront $variable { ... }`
* **context**: `http`

Creates a new variable whose value is a CloudFront token, created according to the 
parameters specified within the block.

The block supports the following parameters:

#### `private_key_file`
* **syntax**: `private_key_file filename`
* **default**: `N/A (mandatory)`

Sets the file name of the private key (PEM file)

#### `key_pair_id`
* **syntax**: `key_pair_id id`
* **default**: `N/A (mandatory)`

Sets the key pair id

#### `acl`
* **syntax**: `acl acl`
* **default**: `$secure_token_baseuri_comma`

Sets the signed part of the URL (ACL). The parameter value can contain variables.

#### `end`
* **syntax**: `end time`
* **default**: `86400`

Sets the end time of the token (see `Time format` below)

#### `ip_address`
* **syntax**: `ip_address address`
* **default**: `none`

Sets the IP address that should be embedded in the token.
The parameter value can contain variables, e.g. $remote_addr/32 can be used to limit the token to the specific IP of the client.

### URI encryption parameters

#### `secure_token_encrypt_uri`
* **syntax**: `secure_token_encrypt_uri on/off`
* **default**: `off`
* **context**: `http`, `server`, `location`

Enables/disables URI encryption

#### `secure_token_encrypt_uri_key`
* **syntax**: `secure_token_encrypt_uri_key key_hex`
* **default**: `none`
* **context**: `http`, `server`, `location`

Sets the encryption key, the key has to be 256 bits (64 hex characters)

#### `secure_token_encrypt_uri_iv`
* **syntax**: `secure_token_encrypt_uri_iv iv_hex`
* **default**: `none`
* **context**: `http`, `server`, `location`

Sets the encryption iv, the iv has to be 128 bits (32 hex characters)

#### `secure_token_encrypt_uri_part`
* **syntax**: `secure_token_encrypt_uri_part expression`
* **default**: `none`
* **context**: `http`, `server`, `location`

An expression that calculates the part of the URL that should be encrypted in regular expression locations.
For non-regular expression locations, the encrypted part is everything following the path defined on the location block.

Example 1:
```
  location /secret_param/([^/]+)/some_other_param/.* {
  	secure_token_encrypt_uri_part $1;
	...
  }
```
  In this configuration, only the value of `secret_param` will be encrypted/decrypted.

Example 2:  
```
  location /base/ {
    ...
  }
```
  In this configuration, everything following /base/ will be encrypted/decrypted.
  
#### `secure_token_encrypt_uri_hash_size`
* **syntax**: `secure_token_encrypt_uri_hash_size size`
* **default**: `8`
* **context**: `http`, `server`, `location`

The size in bytes of hash used to validate the URI after decryption, the value has to be between 0 and 16.

### Time format

Some of the configuration parameters mentioned above, support both absolute timestamps,
and timestamps relative to `now`.
These parameters can be set in the configuration using one of the following formats:
* `epoch` - UNIX timestamp 0 (01/01/1970)
* `max` - UNIX timestamp 2147483647 (18/01/2038)
* `@1481230000` - UNIX timestamp 1481230000 (8/12/2016)
* `10d` / `+10d` - `now` + 10 days
* `-5m` - `now` - 5 minutes

## Sample configurations

### HLS packaging with Akamai tokens
```
	secure_token_akamai $token {
		key 1234;
		acl "$secure_token_baseuri_comma*";
	}

	server {
	
		location ~ ^/hls/p/\d+/(sp/\d+/)?serveFlavor/ {
			vod hls;

			g2o        on;

			secure_token $token;
			secure_token_types application/vnd.apple.mpegurl;
			
			secure_token_expires_time 100d;
			secure_token_query_token_expires_time 1h;

			more_set_headers 'Access-Control-Allow-Headers: *';
			more_set_headers 'Access-Control-Expose-Headers: Server,range,Content-Length,Content-Range';
			more_set_headers 'Access-Control-Allow-Methods: GET, HEAD, OPTIONS';
			more_set_headers 'Access-Control-Allow-Origin: *';
		}
		
	}
```

### HLS packaging with CloudFront tokens
```
http {
...
    secure_token_expires_time 100d; // this is just an example, adjust to your needs
    secure_token_query_token_expires_time 1h; // this is just an example, adjust to your needs	
    
    secure_token_cloudfront $token {
         private_key_file /path/to/pem;
         key_pair_id KEY_ID;
         acl "$scheme://$http_host$secure_token_baseuri_comma*";
    }

    server {
	# serve flavor HLS
	location ~ ^/hls/p/\d+/(sp/\d+/)?serveFlavor/ {
	    vod                             hls;

	    vod_bootstrap_segment_durations     2000;
	    vod_bootstrap_segment_durations     2000;
	    vod_bootstrap_segment_durations     2000;
	    vod_bootstrap_segment_durations     4000;

	    more_set_headers 'Access-Control-Allow-Headers: Origin,Range,Accept-Encoding,Referer,Cache-Control';
	    more_set_headers 'Access-Control-Expose-Headers: Server,Content-Length,Content-Range,Date';
	    more_set_headers 'Access-Control-Allow-Methods: GET, HEAD, OPTIONS';
	    more_set_headers 'Access-Control-Allow-Origin: *';

	    secure_token_types application/vnd.apple.mpegurl application/dash+xml text/xml;
	    secure_token $token;
	}

    }
...
}
```

### HDS packaging with CloudFront tokens
```
	secure_token_cloudfront $token {
		private_key_file /path/to/pem;
		key_pair_id ABCDEF;
		acl "$scheme://$http_host$secure_token_baseuri_comma*";
	}

	server {
	
		location ~ ^/hds/p/\d+/(sp/\d+/)?serveFlavor/ {
			vod hds;
			vod_segment_duration 6000;
			vod_align_segments_to_key_frames on;
			vod_segment_count_policy last_rounded;

			secure_token $token;
			secure_token_types video/f4m;
			
			secure_token_expires_time 100d;
			secure_token_query_token_expires_time 1h;

			more_set_headers 'Access-Control-Allow-Headers: *';
			more_set_headers 'Access-Control-Expose-Headers: Server,range,Content-Length,Content-Range';
			more_set_headers 'Access-Control-Allow-Methods: GET, HEAD, OPTIONS';
			more_set_headers 'Access-Control-Allow-Origin: *';
		}
		
	}
```

### Encrypted HLS with token security on the encryption key

This configuration enables token security while having static URLs for the video segments,
this enables the caching of the segments transparently by proxies.
```
	secure_token_akamai $token {
		key 1234;
		acl "$secure_token_baseuri_comma*";
	}

	server {
	
		location ~ ^/s/hls/enc/p/\d+/(sp/\d+/)?serveFlavor/ {
			vod hls;
			vod_secret_key "password$vod_filepath";

			secure_token $token;
			secure_token_types application/vnd.apple.mpegurl;
			
			secure_token_expires_time 100d;
			secure_token_query_token_expires_time 1h;
			
			secure_token_uri_filename_prefix index;
			secure_token_tokenize_segments off;

			akamai_token_validate $arg___hdnea__;
			akamai_token_validate_key 1234;
			akamai_token_validate_uri_filename_prefix encryption;
			akamai_token_validate_uri_filename_prefix index;
		}
		
	}
```
Note: this configuration requires the module https://github.com/kaltura/nginx-akamai-token-validate-module
in addition to nginx-secure-token-module

### Adding token security on top of an existing HDS/HLS live stream
```
	secure_token_akamai $token {
		key 1234;
		acl "$secure_token_baseuri_comma*";
	}

	server {
	
		location /secure-live/ {
			proxy_pass http://original.live.domain;

			secure_token $token;
			secure_token_types text/xml application/vnd.apple.mpegurl;		
			secure_token_content_type_f4m text/xml;
			
			secure_token_expires_time 100d;
			secure_token_query_token_expires_time 1h;

			akamai_token_validate $arg___hdnea__;
			akamai_token_validate_key 1234;
			akamai_token_validate_strip_token __hdnea__;
		}
	
	}
```
Note: this configuration requires the module https://github.com/kaltura/nginx-akamai-token-validate-module
in addition to nginx-secure-token-module

### URI encryption
```
	location ~ ^/hls/p/\d+/(sp/\d+/)?serveFlavor/entryId/([^/]+)/(.*) {
		vod hls;
		vod_secret_key "password$2";

		secure_token_encrypt_uri on;
		secure_token_encrypt_uri_key 000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f;
		secure_token_encrypt_uri_iv 00000000000000000000000000000000;
		secure_token_encrypt_uri_part $3;
		secure_token_types application/vnd.apple.mpegurl;

		add_header Last-Modified "Sun, 19 Nov 2000 08:52:00 GMT";
		expires 100d;
	}
```

## Nginx variables

The module adds the following Nginx variables:
* `$secure_token_baseuri` - contains the value of the `$uri` built in variable truncated up to the last slash (/). 
	For exmaple, if `$uri` is /a/b/c.htm then `$secure_token_baseuri` will be /a/b/.
* `$secure_token_baseuri_comma` - same as `$secure_token_baseuri`, except that if this value contains a comma (,) 
	the value is truncated up to the comma position.
	For exmaple, if `$uri` is /a/b/c.htm then `$secure_token_baseuri_comma` will be /a/b/; 
	if `$uri` is /a/b,c/d.htm then `$secure_token_baseuri_comma` will be /a/b.
* `$secure_token_original_uri` - contains the original (encrypted) uri when using uri encryption.
	Note that the built in `$uri` variable contains the modified (decrypted) uri in this case.

## Copyright & License

All code in this project is released under the [AGPLv3 license](http://www.gnu.org/licenses/agpl-3.0.html) unless a different license for a particular library is specified in the applicable library path. 

Copyright © Kaltura Inc. All rights reserved.
