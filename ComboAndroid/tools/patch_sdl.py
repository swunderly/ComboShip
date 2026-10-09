"""Narrow API-35 compatibility edits to the pinned SDL Java sources after copying them."""
from pathlib import Path

def patch(java: Path):
    def replace(file, old, new):
        path=java/'org/libsdl/app'/file
        text=path.read_text(encoding='utf-8')
        if new in text:return
        if text.count(old)!=1:raise RuntimeError(f'SDL patch anchor changed in {file}')
        path.write_text(text.replace(old,new),encoding='utf-8',newline='\n')
    replace('HIDDeviceManager.java',
        '        mContext.registerReceiver(mUsbBroadcast, filter);',
        '''        // ComboShip Android: protect the app's USB permission action on all supported API levels.
        androidx.core.content.ContextCompat.registerReceiver(mContext, mUsbBroadcast, filter,
                androidx.core.content.ContextCompat.RECEIVER_NOT_EXPORTED);''')
    replace('SDLAudioManager.java',
        '''        if (isCapture) {
            if (mAudioRecord == null) {''',
        '''        if (isCapture) {
            // ComboShip does not request microphone access. Reject an unexpected capture request.
            if (mContext == null || mContext.checkSelfPermission(android.Manifest.permission.RECORD_AUDIO)
                    != android.content.pm.PackageManager.PERMISSION_GRANTED) {
                return null;
            }
            if (mAudioRecord == null) {''')

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('java',type=Path)
    patch(parser.parse_args().java)
