#pragma once

// Import the COM type library embedded in the installed DeckLink driver.
// Requires Blackmagic Desktop Video runtime on the build/run machine.
#ifdef _MSC_VER
#import "C:/Program Files/Blackmagic Design/Desktop Video/DeckLinkAPI64.dll" \
    no_namespace \
    named_guids \
    exclude("GetCommandLine", "GetObject")
#else
#error "MSVC is required to import the DeckLink type library on Windows"
#endif
