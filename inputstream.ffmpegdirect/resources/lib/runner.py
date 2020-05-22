import xbmc
import xbmcaddon
import xbmcvfs

def hidden(path):
    return path.startswith('.') or path.startswith('_UNPACK')

ADDON = xbmcaddon.Addon()
timeshiftBufferPath = ADDON.getSetting('timeshiftBufferPath')

# Add a trailing slash if we don't have as it's required to test if a directory exists
if not timeshiftBufferPath.endswith("/"):
    timeshiftBufferPath += "/"

if xbmcvfs.exists(timeshiftBufferPath):
    dirs, files = xbmcvfs.listdir(timeshiftBufferPath)
    # xbmcvfs bug: sometimes return invalid utf-8 encoding. we only care about
    # finding changed paths so it's ok to ignore here.

    #dirs = [timeshiftBufferPath + _ for _ in dirs if not hidden(_)]
    files = [timeshiftBufferPath + _ for _ in files if not hidden(_)]

    # for d in dirs:
    #     xbmcvfs.rmdir(d, true)
    for f in files:
        if f.endswith(".idx") or f.endswith(".seg"):
            xbmcvfs.delete(f)

