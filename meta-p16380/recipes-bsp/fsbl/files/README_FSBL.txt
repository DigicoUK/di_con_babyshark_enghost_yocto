In it's previous state, this project only required an XSA in order to build the
FSBL. Since the fsbl should never (or almost never) change, I have baked it in
to the build. If we ever need to re-generate the fsbl for some reason, it can
be generated via vitis without much trouble.
