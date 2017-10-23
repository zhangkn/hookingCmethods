# hookingCmethods
Hooking &amp; Executing Code with dlopen &amp; dlsym ---Easy mode:hooking C methods

Easy mode: hooking C functions

Setting up your project
This project is very simple. All it does is display a watermarked image in a UIImageView.

Easy mode: hooking C functions
you’ll be going after the getenv C function. This simple C function takes a char * (null terminated string) for input and returns the environment variable for the parameter you supply.

Open and launch the Watermark project in Xcode. Create a new symbolic breakpoint, putting getenv in the Symbol section. Next, add a custom action with the following:

po (char *)$rdi
Now, make sure the execution automatically continues after the breakpoint hits.

Finally, build and run the application on the iPhone Simulator then watch the console. You’ll get a slew of output indicating this method is called quite frequently. It’ll look similar to the following:

"DYLD_INSERT_LIBRARIES"

"NSZombiesEnabled"

"OBJC_DEBUG_POOL_ALLOCATION"

"MallocStackLogging"

"MallocStackLoggingNoCompact"

"OBJC_DEBUG_MISSING_POOLS"

"LIBDISPATCH_DEBUG_QUEUE_INVERSIONS"

"LIBDISPATCH_CONTINUATION_ALLOCATOR"

"XPC_FLAGS"

"XPC_NULL_BOOTSTRAP"

"XBS_IS_CHROOTED"

"XPC_SERVICES_UNAVAILABLE"

"XPC_SIMULATOR_LAUNCHD_NAME"

"CLASSIC"

"LANG"

"XPC_SIMULATOR_LAUNCHD_NAME"

you want to have the function hooked before your application starts up, you’ll need to create a dynamic framework to put the hooking logic in so it’ll be available before the main function is called. You’ll explore this easy case of hooking a C function inside your own executable first.

Open AppDelegate.swift, and replace
application(_:didFinishLaunchingWithOptions:) with the following:

 func application(_ application: UIApplication,
                     didFinishLaunchingWithOptions launchOptions:
        [UIApplicationLaunchOptionsKey : Any]? = nil) -> Bool {
        if let cString = getenv("HOME") {
            let homeEnv = String(cString: cString)
            print("HOME env: \(homeEnv)")
        }
        return true
    }
This creates a call to getenv to get the HOME environment variable.
Next, remove the symbolic getenv breakpoint you previously created and build and
run the application.
The console output will look similar to the following:

"HOME"

HOME env: /Users/devzkn/Library/Developer/CoreSimulator/Devices/2499648A-9622-42D9-8931-C342DB0208CC/data/Containers/Data/Application/81D1A334-A8C2-498E-87E9-950626126918
In Xcode, navigate to FileNewTarget and select Cocoa Touch Framework. Choose HookingC as the product name, and set the language to Objective-C.

Once this new framework is created, create a new C file. In Xcode, select FileNewFile, then select C file. Name this file getenvhook. Uncheck the checkbox for Also create a header file. Save the file with the rest of the project.

Make sure this file belongs to the HookingC framework that you’ve just created, and not Watermark.

Open getenvhook.c and replace its contents with the following:

import <dlfcn.h>
import <assert.h>
import <stdio.h>
import <dispatch/dispatch.h>
import <string.h>
/**
 
 • dlfcn.h will be responsible for two very interesting functions: dlopen and dlsym.
 • assert.h the assert will test the library containing the real getenv is correctly loaded.
 • stdio.h will be used temporarily for a C printf call.
 • dispatch.h will be used to to properly set up the logic for GCD’s dispatch_once
 function.
 • string.h will be used for the strcmp function, which compares two C strings.
 */
Next, redeclare the getenv function with the hard-coded stub shown below:

char * getenv(const char *name) {
    return "YAY!";
}
Finally, build and run your application to see what happens. You’ll get the following output:
HOME env: YAY!

HOME env: YAY!
"IPHONE_SIMULATOR_ROOT"

 IPHONE_SIMULATOR_ROOT: YAY!
Awesome! You were able to successfully replace this method with your own function.

However, this isn’t quite what you want. You want to call the original getenv function and augment the return value if "HOME" is supplied as input.

What would happen if you tried to call the original getenv function inside your getenv function? Try it out and see what happens. Add some temporary code so the getenv looks like the following:

char * getenv(const char *name) {
    return getenv(name);
    return "YAY!";
}
如果要在Terminal进行lldb的话，就关闭Xcode的运行，使用模拟器单独启动Watermark

如果要在Xcode进行lldb 的话，可以设置个断点即可

devzkndeMacBook-Pro:~ devzkn$ lldb -n Watermark
 image lookup -s getenv
1 symbols match 'getenv' in /Users/devzkn/Library/Developer/Xcode/DerivedData/Watermark-eqvbswmyqiyhvgbqmiqzpnnzylsn/Build/Products/Debug-iphonesimulator/Watermark.app/Frameworks/HookingC.framework/HookingC:
        Address: HookingC[0x0000000000000f60] (HookingC.__TEXT.__text + 0)
        Summary: HookingC`getenv at getenvhook.c:29
1 symbols match 'getenv' in /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk//usr/lib/system/libsystem_c.dylib:
        Address: libsystem_c.dylib[0x000000000005ca26] (libsystem_c.dylib.__TEXT.__text + 375174)
        Summary: libsystem_c.dylib`getenv
You’ll get two hits. One of them will be the getenv function you created yourself. More importantly, you’ll get the location of the getenv function you actually care about. It looks like this function is located in libsystem_c.dylib, and its full path is at /usr/lib/system/libsystem_c.dylib. Remember, the simulator prepends that big long path to these directories, but the dynamic linker is smart enough to search in the correct areas. Everything after iPhoneSimulator.sdk is where this framework is actually stored on a real iOS device.

Now you know exactly where this function is loaded, it’s time to whip out the first of the amazing “dl” duo, dlopen. Its function signature looks like the following:

extern void * dlopen(const char * __path, int __mode);
extern void * dlsym(void * __handle, const char * __symbol);```



dlopen expects a fullpath in the form of a char * and a second parameter, which is a mode expressed as an integer that determines how dlopen should load the module. If successful, dlopen returns an opaque handle (a void *) ,or NULL if it fails.
After dlopen (hopefully) returns a reference to the module, you’ll use dlsym to get a reference to the getenv function. dlsym has the following function signature:
  extern void * dlsym(void * __handle, const char * __symbol);
dlsym expects to take the reference generated by dlopen as the first parameter and the name of the function as the second parameter. If everything goes well, dlsym will return the function address for the symbol specified in the second parameter or NULL if it failed.
Replace your getenv function with the following:
char getenv(const char name) {
void *handle = dlopen("/usr/lib/system/libsystem_c.dylib", RTLD_NOW);
assert(handle);
void *real_getenv = dlsym(handle, "getenv");
printf("Real getenv: %pnFake getenv: %pn", real_getenv, getenv);
return "YAY!";
}

You used the RTLD_NOW mode of dlopen to say, “Hey, don’t wait or do any cute lazy loading stuff. Open this module right now.” After making sure the handle is not NULL through a C assert, you call dlsym to get a handle on the “real” getenv.
Build and run the application. You’ll get output similar to the following:
Real getenv: 0x10fde4a26
Fake getenv: 0x10d081df0



Right now, the real_getenv function pointer is void *, meaning it could be anything. You already know the function signature of getenv, so you can simply cast it to that.
Replace your getenv function one last time with the following:
char getenv(const char name) {
static void *handle; // 1
static char (real_getenv)(const char *); // 2
static dispatch_once_t onceToken;
dispatch_once(&onceToken, ^{ // 3

handle = dlopen("/usr/lib/system/libsystem_c.dylib", RTLD_NOW);
assert(handle);
real_getenv = dlsym(handle, "getenv");
});
if (strcmp(name, "HOME") == 0) { // 4
return "/"; }
return real_getenv(name); // 5
}


You might not be used to this amount of C code, so let’s break it down:
1. Thiscreatesastaticvariablenamedhandle.It’sstaticsothisvariablewill survive the scope of the function. That is, this variable will not be erased when the function exits, but you’ll only be able to access it inside the getenv function.
2. You’re doing the same thing here as you declare the real_getenv variableas static, but you’ve made other changes to the real_getenv function pointer. You’ve cast this function pointer to correctly match the signature of getenv. This will allow you to call the real getenv function through the real_getenv variable. Cool, right?
3. You’re using GCD’s dispatch_once because you really only need to call the setup once. This nicely complements the static variables you declared a couple lines
 above. You don’t want to be doing the lookup logic every time your augmented
getenv runs!
4. You’re using C’s strcmp to see ifyou’re querying the "HOME" environment variable. If it’s true, you’re simply returning "/" to signify the root directory. Essentially, you’re overriding what the getenv function returns.
5. If "HOME" is not supplied as an input parameter,then just fallback on the default getenv.

HOME env: /
IPHONE_SIMULATOR_ROOT: /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk
