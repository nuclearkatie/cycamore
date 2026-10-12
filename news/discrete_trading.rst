**Added:**

* Added ``discrete_trading`` to Storage, which trades each resource object in stocks whole, as an exclusive bid of its own quantity, oldest first; objects that do not fit a request are skipped
* Added ``keep_packaging`` to Storage, which preserves the packaging of incoming material so that, with ``discrete_handling`` and ``discrete_trading``, packaged material is traded away in its original package

**Changed:** None

**Deprecated:** None

**Removed:** None

**Fixed:** None

**Security:** None
