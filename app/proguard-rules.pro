-dontwarn org.bouncycastle.jsse.BCSSLParameters
-dontwarn org.bouncycastle.jsse.BCSSLSocket
-dontwarn org.bouncycastle.jsse.provider.BouncyCastleJsseProvider
-dontwarn org.conscrypt.Conscrypt$Version
-dontwarn org.conscrypt.Conscrypt
-dontwarn org.conscrypt.ConscryptHostnameVerifier
-dontwarn org.openjsse.javax.net.ssl.SSLParameters
-dontwarn org.openjsse.javax.net.ssl.SSLSocket
-dontwarn org.openjsse.net.ssl.OpenJSSE
-dontwarn java.beans.Introspector
-dontwarn java.beans.VetoableChangeListener
-dontwarn java.beans.VetoableChangeSupport
-dontwarn java.beans.BeanInfo
-dontwarn java.beans.IntrospectionException
-dontwarn java.beans.PropertyDescriptor

# Keep ini4j Service Provider Interface
-keep,allowobfuscation,allowoptimization class org.ini4j.spi.** { *; }

# Keep native methods and JNI classes
-keep class me.yuki.spectre.Natives {
    *;
}

-keepclasseswithmembernames class * {
    native <methods>;
}

-keep class me.yuki.spectre.Natives$Profile { *; }
-keep class me.yuki.spectre.Natives$KPMCtlRes { *; }

# Keep RootServices
-keep class me.yuki.spectre.services.RootServices { *; }

# Keep AIDL interfaces
-keep class me.yuki.spectre.IAPRootService { *; }
-keep class me.yuki.spectre.IAPRootService$Stub { *; }
-keep class rikka.parcelablelist.ParcelableListSlice { *; }
# Keep ScriptInfo for Gson serialization in release
-keep class me.yuki.spectre.data.ScriptInfo { *; }
-keepclassmembers class me.yuki.spectre.data.ScriptInfo { *; }

# Gson
-keepattributes Signature
-keepattributes *Annotation*
-keep class sun.misc.Unsafe { *; }
-keep class com.google.gson.** { *; }
-keep class * extends com.google.gson.reflect.TypeToken

# Kotlin
-assumenosideeffects class kotlin.jvm.internal.Intrinsics {
    public static void check*(...);
    public static void throw*(...);
}

-repackageclasses 'a'
-allowaccessmodification
-overloadaggressively
-renamesourcefileattribute ""

# Strip all debug/source info to minimize string exposure
-dontnote **
-dontwarn **

# Aggressive obfuscation: use short dictionary for class/method/field names
-obfuscationdictionary proguard-dict.txt
-classobfuscationdictionary proguard-dict.txt
-packageobfuscationdictionary proguard-dict.txt

# Remove all Log calls in release builds to prevent logcat exposure
-assumenosideeffects class android.util.Log {
    public static int v(...);
    public static int d(...);
    public static int i(...);
    public static int w(...);
}
