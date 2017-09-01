#### The libcurl make definition
CURL_VERSION = 7.50.3
CURL_SRC_DIR = curl-$(CURL_VERSION)
CURL_SRC_TAR_PACKAGE_NAME = curl-$(CURL_VERSION).tar.gz
CURL_TARGET_DIR = $(PWD)/curl

.PHONY:clean_curl

curl: $(CURL_SRC_DIR)
	@cd $(CURL_SRC_DIR); \
		./configure --prefix=$(CURL_TARGET_DIR); \
		make; \
		make install
        
$(CURL_SRC_DIR): $(CURL_SRC_TAR_PACKAGE_NAME)
	@tar -xvf $(CURL_SRC_TAR_PACKAGE_NAME)
        
clean_curl:
	@mv -f $(CURL_TARGET_DIR) $(CURL_TARGET_DIR).bak


#### The zlib make definition
ZLIB_VERSION = 1.2.8
ZLIB_SRC_DIR = zlib-$(ZLIB_VERSION)
ZLIB_SRC_TAR_PACKAGE_NAME = zlib-$(ZLIB_VERSION).tar.gz
ZLIB_TARGET_DIR = $(PWD)/zlib

.PHONY:clean_zlib

zlib: $(ZLIB_SRC_DIR)
	@cd $(ZLIB_SRC_DIR); \
		./configure --prefix=$(ZLIB_TARGET_DIR); \
		make; \
		make install
        
$(ZLIB_SRC_DIR): $(ZLIB_SRC_TAR_PACKAGE_NAME)
	@tar -xvf $(ZLIB_SRC_TAR_PACKAGE_NAME)
        
clean_zlib:
	@mv -f $(ZLIB_TARGET_DIR) $(ZLIB_TARGET_DIR).bak

#########################

distclean:clean_zlib clean_curl
	@$(MAKE) clean -C ./jsoncpp-src-0.5.0/makefiles
	
deps:curl zlib
	@$(MAKE) all -C ./jsoncpp-src-0.5.0/makefiles
