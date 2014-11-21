// AUTOGENERATED FILE - DO NOT MODIFY!
// This file generated by Djinni from exception.djinni

#import "DBCppExceptionCppProxy+Private.h"
#import "DBCppException.h"
#import "DBCppExceptionCppProxy+Private.h"
#import "DJIError.h"
#include <exception>
#include <utility>

static_assert(__has_feature(objc_arc), "Djinni requires ARC to be enabled for this file");

@implementation DBCppExceptionCppProxy

- (id)initWithCpp:(const std::shared_ptr<CppException> &)cppRef
{
    if (self = [super init]) {
        _cppRef = cppRef;
    }
    return self;
}

- (void)dealloc
{
    djinni::DbxCppWrapperCache<CppException> & cache = djinni::DbxCppWrapperCache<CppException>::getInstance();
    cache.remove(_cppRef);
}

+ (id)cppExceptionWithCpp:(const std::shared_ptr<CppException> &)cppRef
{
    djinni::DbxCppWrapperCache<CppException> & cache = djinni::DbxCppWrapperCache<CppException>::getInstance();
    return cache.get(cppRef, [] (const std::shared_ptr<CppException> & p) { return [[DBCppExceptionCppProxy alloc] initWithCpp:p]; });
}

- (int32_t)throwAnException {
    try {
        int32_t cppRet = _cppRef->throw_an_exception();
        int32_t objcRet = cppRet;
        return objcRet;
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

+ (id <DBCppException>)get {
    try {
        std::shared_ptr<CppException> cppRet = CppException::get();
        id <DBCppException> objcRet = [DBCppExceptionCppProxy cppExceptionWithCpp:cppRet];
        return objcRet;
    } DJINNI_TRANSLATE_EXCEPTIONS()
}

@end