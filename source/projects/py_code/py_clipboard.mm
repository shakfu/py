// SPDX-License-Identifier: MIT
/* py_clipboard.mm -- system clipboard through NSPasteboard. The Max SDK has no
 * clipboard API. Not ARC: explicit @autoreleasepool and release. */

#include "py_clipboard.h"

#import <AppKit/AppKit.h>

namespace py_code {

std::string clipboard_read_text() {
	@autoreleasepool {
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		NSString *s = [pb stringForType:NSPasteboardTypeString];
		if (!s)
			return std::string();
		const char *utf8 = [s UTF8String];
		return utf8 ? std::string(utf8) : std::string();
	}
}

bool clipboard_write_text(const std::string &text) {
	@autoreleasepool {
		NSString *s = [[NSString alloc] initWithBytes:text.data()
											  length:(NSUInteger)text.size()
											encoding:NSUTF8StringEncoding];
		if (!s)
			return false;
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		[pb clearContents];
		const BOOL ok = [pb setString:s forType:NSPasteboardTypeString];
		[s release];
		return ok == YES;
	}
}

} // namespace py_code
