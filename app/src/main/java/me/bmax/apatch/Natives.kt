package me.bmax.apatch

import android.os.Parcelable
import android.content.Context
import androidx.annotation.Keep
import androidx.compose.runtime.Immutable
import dalvik.annotation.optimization.FastNative
import kotlinx.parcelize.Parcelize

object Natives {
    init {
        System.loadLibrary("sysjni")
    }

    @Immutable
    @Parcelize
    @Keep
    data class Profile(
        var uid: Int = 0,
        var toUid: Int = 0,
        var scontext: String = APApplication.DEFAULT_SCONTEXT,
    ) : Parcelable

    @Keep
    class KPMCtlRes {
        var rc: Long = 0
        var outMsg: String? = null

        constructor()

        constructor(rc: Long, outMsg: String?) {
            this.rc = rc
            this.outMsg = outMsg
        }
    }


    @FastNative
    private external fun nativeSu(superKey: String, toUid: Int, scontext: String?): Long

    fun su(toUid: Int, scontext: String?): Boolean {
        return nativeSu(APApplication.superKey, toUid, scontext) == 0L
    }

    fun su(): Boolean {
        return su(0, "")
    }

    @FastNative
    external fun nativeReady(superKey: String): Boolean

    @FastNative
    private external fun nativeSuPath(superKey: String): String

    fun suPath(): String {
        return nativeSuPath(APApplication.superKey)
    }

    @FastNative
    private external fun nativeSuUids(superKey: String): IntArray

    fun suUids(): IntArray {
        return nativeSuUids(APApplication.superKey)
    }

    @FastNative
    private external fun nativeCoreVersion(superKey: String): Long
    fun coreVersion(): Long {
        return nativeCoreVersion(APApplication.superKey)
    }

    @FastNative
    private external fun nativeCoreBuildTime(superKey: String): String
    fun coreBuildTime(): String {
        return nativeCoreBuildTime(APApplication.superKey)
    }

    private external fun nativeLoadModule(
        superKey: String, modulePath: String, args: String
    ): Long

    fun loadModule(modulePath: String, args: String): Long {
        return nativeLoadModule(APApplication.superKey, modulePath, args)
    }

    private external fun nativeUnloadModule(superKey: String, moduleName: String): Long
    fun unloadModule(moduleName: String): Long {
        return nativeUnloadModule(APApplication.superKey, moduleName)
    }

    @FastNative
    private external fun nativeModuleNum(superKey: String): Long

    fun moduleNum(): Long {
        return nativeModuleNum(APApplication.superKey)
    }

    @FastNative
    private external fun nativeModuleList(superKey: String): String
    fun moduleList(): String {
        return nativeModuleList(APApplication.superKey)
    }

    @FastNative
    private external fun nativeModuleInfo(superKey: String, moduleName: String): String
    fun moduleInfo(moduleName: String): String {
        return nativeModuleInfo(APApplication.superKey, moduleName)
    }

    private external fun nativeControlModule(
        superKey: String, modName: String, jctlargs: String
    ): KPMCtlRes

    fun moduleControl(moduleName: String, controlArg: String): KPMCtlRes {
        return nativeControlModule(APApplication.superKey, moduleName, controlArg)
    }

    @FastNative
    private external fun nativeGrantSu(
        superKey: String, uid: Int, toUid: Int, scontext: String?
    ): Long

    fun grantSu(uid: Int, toUid: Int, scontext: String?): Long {
        return nativeGrantSu(APApplication.superKey, uid, toUid, scontext)
    }

    @FastNative
    private external fun nativeRevokeSu(superKey: String, uid: Int): Long
    fun revokeSu(uid: Int): Long {
        return nativeRevokeSu(APApplication.superKey, uid)
    }

    @FastNative
    private external fun nativeSetUidExclude(superKey: String, uid: Int, exclude: Int): Int
    fun setUidExclude(uid: Int, exclude: Int): Int {
        return nativeSetUidExclude(APApplication.superKey, uid, exclude)
    }

    @FastNative
    private external fun nativeGetUidExclude(superKey: String, uid: Int): Int
    fun isUidExcluded(uid: Int): Int {
        return nativeGetUidExclude(APApplication.superKey, uid)
    }

    @FastNative
    private external fun nativeSuProfile(superKey: String, uid: Int): Profile
    fun suProfile(uid: Int): Profile {
        return nativeSuProfile(APApplication.superKey, uid)
    }

    @FastNative
    private external fun nativeResetSuPath(superKey: String, path: String): Boolean
    fun resetSuPath(path: String): Boolean {
        return nativeResetSuPath(APApplication.superKey, path)
    }

    external fun nativeGetApiToken(context: Context): String
    fun getApiToken(context: Context): String {
        return nativeGetApiToken(context)
    }
}
