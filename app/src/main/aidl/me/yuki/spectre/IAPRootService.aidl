// IAPRootService.aidl
package me.yuki.spectre;

import android.content.pm.PackageInfo;
import rikka.parcelablelist.ParcelableListSlice;

interface IAPRootService {
    ParcelableListSlice<PackageInfo> getPackages(int flags);
}