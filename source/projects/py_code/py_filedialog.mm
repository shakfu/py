// SPDX-License-Identifier: MIT
/* py_filedialog.mm -- NSSavePanel/NSOpenPanel for `.py` files.
 *
 * Not ARC: Max loads the bundle into its own process, so every call runs in an
 * explicit @autoreleasepool. UTType needs macOS 11; older systems fall back to
 * the extension list API. */

#include "py_filedialog.h"

#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

namespace py_code {

static NSString *utf8_to_ns(const std::string &s) {
	return [[[NSString alloc] initWithBytes:s.data() length:(NSUInteger)s.size()
								   encoding:NSUTF8StringEncoding] autorelease];
}

static std::string ns_to_utf8(NSString *s) {
	const char *utf8 = s ? [s UTF8String] : NULL;
	return utf8 ? std::string(utf8) : std::string();
}

static void restrict_to_py(NSSavePanel *panel) {
	if (@available(macOS 11.0, *)) {
		UTType *py = [UTType typeWithFilenameExtension:@"py"];
		if (py)
			[panel setAllowedContentTypes:@[ py ]];
	} else {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
		[panel setAllowedFileTypes:@[ @"py" ]];
#pragma clang diagnostic pop
	}
}

std::string dialog_save_code(const std::string &default_name) {
	@autoreleasepool {
		NSSavePanel *panel = [NSSavePanel savePanel];
		if (!panel)
			return std::string();
		[panel setTitle:@"Save Python code"];
		[panel setCanCreateDirectories:YES];
		[panel setExtensionHidden:NO];
		restrict_to_py(panel);
		[panel setAllowsOtherFileTypes:NO];
		if (NSString *name = utf8_to_ns(default_name))
			[panel setNameFieldStringValue:name];
		if ([panel runModal] != NSModalResponseOK)
			return std::string();
		return ns_to_utf8([[panel URL] path]);
	}
}

std::string dialog_open_code() {
	@autoreleasepool {
		NSOpenPanel *panel = [NSOpenPanel openPanel];
		if (!panel)
			return std::string();
		[panel setTitle:@"Open Python code"];
		[panel setCanChooseFiles:YES];
		[panel setCanChooseDirectories:NO];
		[panel setAllowsMultipleSelection:NO];
		[panel setResolvesAliases:YES];
		restrict_to_py(panel);
		[panel setAllowsOtherFileTypes:YES];
		if ([panel runModal] != NSModalResponseOK)
			return std::string();
		NSURL *url = [[panel URLs] firstObject];
		return url ? ns_to_utf8([url path]) : std::string();
	}
}

} // namespace py_code
