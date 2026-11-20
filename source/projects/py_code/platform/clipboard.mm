// SPDX-License-Identifier: MIT
/* clipboard.mm -- see clipboard.h. Manual reference counting (no ARC). */

#include "platform/clipboard.h"

#import <AppKit/AppKit.h>

std::string pyc::clipboard_text() {
	std::string out;
	@autoreleasepool {
		NSString *s = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
		const char *u = s ? [s UTF8String] : NULL;
		if (u)
			out = u;
	}
	return out;
}

void pyc::set_clipboard_text(const std::string &s) {
	@autoreleasepool {
		NSString *ns = [[[NSString alloc] initWithBytes:s.data() length:s.size() encoding:NSUTF8StringEncoding]
			autorelease];
		if (!ns)
			return;
		NSPasteboard *pb = [NSPasteboard generalPasteboard];
		[pb clearContents];
		[pb setString:ns forType:NSPasteboardTypeString];
	}
}
