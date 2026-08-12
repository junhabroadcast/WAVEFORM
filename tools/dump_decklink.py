import comtypes.gen.DeckLinkAPI as d


def dump_iface(name: str) -> None:
    iface = getattr(d, name)
    print("===", name, iface._iid_)
    methods = getattr(iface, "_methods_", None)
    if methods:
        for m in methods:
            print(" ", m)
    else:
        for attr in dir(iface):
            if not attr.startswith("_"):
                print(" ", attr)
    print()


for n in [
    "IDeckLinkIterator",
    "IDeckLink",
    "IDeckLinkInput",
    "IDeckLinkInputCallback",
    "IDeckLinkVideoInputFrame",
    "IDeckLinkDisplayMode",
    "IDeckLinkDisplayModeIterator",
]:
    dump_iface(n)

for n in [
    "bmdFrameFlagDefault",
    "bmdVideoInputFlagDefault",
    "bmdFrameHasNoInputSource",
    "bmdModeNTSC",
    "bmdModeHD1080i5994",
    "bmdModeHD1080p5994",
    "bmdModeHD1080p6000",
    "bmdModeHD720p5994",
    "bmdFormat10BitYUV",
    "bmdFormat8BitYUV",
]:
    if hasattr(d, n):
        val = getattr(d, n)
        print(n, "=", hex(val) if isinstance(val, int) else val)
