//
//  getenvhook.c
//  Watermark
//
//  Created by devzkn on 23/10/2017.
//  Copyright © 2017 Weiliu. All rights reserved.
//

//#include <stdio.h>
/**
 
 • dlfcn.h will be responsible for two very interesting functions: dlopen and dlsym.
 • assert.h the assert will test the library containing the real getenv is correctly loaded.
 • stdio.h will be used temporarily for a C printf call.
 • dispatch.h will be used to to properly set up the logic for GCD’s dispatch_once
 function.
 • string.h will be used for the strcmp function, which compares two C strings.
 */
#import <dlfcn.h>
#import <assert.h>
#import <stdio.h>
#import <dispatch/dispatch.h>
#import <string.h>

//char * getenv(const char *name) {
//    return "YAY!";
//}

//char * getenv(const char *name) {
////    return getenv(name);
//    return "YAY!";
//}
/**
 
 (lldb)  image lookup -s getenv
 1 symbols match 'getenv' in /Users/devzkn/Library/Developer/Xcode/DerivedData/Watermark-eqvbswmyqiyhvgbqmiqzpnnzylsn/Build/Products/Debug-iphonesimulator/Watermark.app/Frameworks/HookingC.framework/HookingC:
 Address: HookingC[0x0000000000000f60] (HookingC.__TEXT.__text + 0)
 Summary: HookingC`getenv at getenvhook.c:29
 1 symbols match 'getenv' in /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk//usr/lib/system/libsystem_c.dylib:
 Address: libsystem_c.dylib[0x000000000005ca26] (libsystem_c.dylib.__TEXT.__text + 375174)
 Summary: libsystem_c.dylib`getenv

// */
//char * getenv(const char *name) {
//    void *handle = dlopen("/usr/lib/system/libsystem_c.dylib", RTLD_NOW);// used the RTLD_NOW mode of dlopen to say, “Hey, don’t wait or do any cute lazy loading stuff. Open this module right now.”
//    assert(handle);// After making sure the handle is not NULL through a C assert, you call dlsym to get a handle on the “real” getenv.
//    void *real_getenv = dlsym(handle, "getenv");
//    printf("Real getenv: %p\nFake getenv: %p\n", real_getenv, getenv);
//    return "YAY!";
//}

/**
 This will allow you to call the real getenv function through the real_getenv variable
 
 
 You might not be used to this amount of C code, so let’s break it down:
 1. Thiscreatesastaticvariablenamedhandle.It’sstaticsothisvariablewill survive the scope of the function. That is, this variable will not be erased when the function exits, but you’ll only be able to access it inside the getenv function.
 2. You’re doing the same thing here as you declare the real_getenv variableas static, but you’ve made other changes to the real_getenv function pointer. You’ve cast this function pointer to correctly match the signature of getenv. This will allow you to call the real getenv function through the real_getenv variable. Cool, right?
 3. You’re using GCD’s dispatch_once because you really only need to call the setup once. This nicely complements the static variables you declared a couple lines
 above. You don’t want to be doing the lookup logic every time your augmented
 getenv runs!
 4. You’re using C’s strcmp to see ifyou’re querying the "HOME" environment variable. If it’s true, you’re simply returning "/" to signify the root directory. Essentially, you’re overriding what the getenv function returns.
 5. If "HOME" is not supplied as an input parameter,then just fallback on the default getenv.

 */
char * getenv(const char *name) {
    static void *handle;      // 1Thiscreatesastaticvariablenamedhandle.It’sstaticsothisvariablewill survive the scope of the function. That is, this variable will not be erased when the function exits, but you’ll only be able to access it inside the getenv function.
    static char * (*real_getenv)(const char *); // 2
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{  // 3
        handle = dlopen("/usr/lib/system/libsystem_c.dylib", RTLD_NOW);
        assert(handle);
        real_getenv = dlsym(handle, "getenv");
    });
    if (strcmp(name, "HOME") == 0) { // 4
        return "/"; }
    return real_getenv(name); // 5//这样就不会导致自己调用自己
}

