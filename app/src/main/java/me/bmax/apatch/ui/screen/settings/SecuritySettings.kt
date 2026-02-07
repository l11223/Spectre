package me.bmax.apatch.ui.screen.settings

import android.widget.Toast
import androidx.biometric.BiometricPrompt
import androidx.compose.foundation.clickable
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Fingerprint
import androidx.compose.material.icons.filled.Key
import androidx.compose.material.icons.filled.KeyOff
import androidx.compose.material.icons.filled.Security
import androidx.compose.material.icons.filled.VerifiedUser
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import me.bmax.apatch.APApplication
import me.bmax.apatch.R
import me.bmax.apatch.ui.component.SettingsCategory
import me.bmax.apatch.ui.component.SwitchItem
import me.bmax.apatch.ui.component.rememberConfirmDialog
import me.bmax.apatch.util.APatchKeyHelper

@Composable
fun SecuritySettings(
    searchText: String,
    snackBarHost: SnackbarHostState,
    kPatchReady: Boolean
) {
    val prefs = APApplication.sharedPreferences
    val context = LocalContext.current
    val activity = context as? FragmentActivity

    // Security Category
    val securityTitle = stringResource(R.string.settings_category_security)
    val matchSecurity = shouldShow(searchText, securityTitle)

    // Biometric Login State
    var biometricLogin by remember { mutableStateOf(prefs.getBoolean("biometric_login", false)) }

    // Biometric
    val biometricManager = androidx.biometric.BiometricManager.from(context)
    val canAuthenticate = biometricManager.canAuthenticate(
        androidx.biometric.BiometricManager.Authenticators.BIOMETRIC_STRONG or
                androidx.biometric.BiometricManager.Authenticators.DEVICE_CREDENTIAL
    ) == androidx.biometric.BiometricManager.BIOMETRIC_SUCCESS

    val biometricTitle = stringResource(id = R.string.settings_biometric_login)
    val biometricSummary = stringResource(id = R.string.settings_biometric_login_summary)
    val showBiometric = canAuthenticate && (matchSecurity || shouldShow(searchText, biometricTitle, biometricSummary))

    // Strong Biometric
    val strongBiometricTitle = stringResource(id = R.string.settings_strong_biometric)
    val strongBiometricSummary = stringResource(id = R.string.settings_strong_biometric_summary)
    val showStrongBiometric = biometricLogin && canAuthenticate && (matchSecurity || shouldShow(searchText, strongBiometricTitle, strongBiometricSummary))

    // Clear SuperKey
    val clearSuperKeyTitle = stringResource(id = R.string.clear_super_key)
    val showClearSuperKey = kPatchReady && (matchSecurity || shouldShow(searchText, clearSuperKeyTitle))

    // Don't Store SuperKey
    val noStoreSuperKeyTitle = stringResource(id = R.string.settings_donot_store_superkey)
    val noStoreSuperKeySummary = stringResource(id = R.string.settings_donot_store_superkey_summary)
    val showNoStoreSuperKey = kPatchReady && (matchSecurity || shouldShow(searchText, noStoreSuperKeyTitle, noStoreSuperKeySummary))

    val showSecurityCategory = showBiometric || showStrongBiometric || showClearSuperKey || showNoStoreSuperKey

    if (showSecurityCategory) {
        SettingsCategory(icon = Icons.Filled.Security, title = securityTitle, isSearching = searchText.isNotEmpty()) {
            
            // Biometric Authentication
            if (showBiometric) {
                SwitchItem(
                    icon = Icons.Filled.Fingerprint,
                    title = biometricTitle,
                    summary = biometricSummary,
                    checked = biometricLogin,
                    onCheckedChange = { checked ->
                        if (!checked) {
                            // Require verification to disable
                            if (activity != null) {
                                val executor = ContextCompat.getMainExecutor(context)
                                val biometricPrompt = BiometricPrompt(activity, executor,
                                    object : BiometricPrompt.AuthenticationCallback() {
                                        override fun onAuthenticationSucceeded(result: BiometricPrompt.AuthenticationResult) {
                                            super.onAuthenticationSucceeded(result)
                                            biometricLogin = false
                                            prefs.edit().putBoolean("biometric_login", false).apply()
                                        }
                                        
                                        override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                                            super.onAuthenticationError(errorCode, errString)
                                            // Toast.makeText(context, errString, Toast.LENGTH_SHORT).show()
                                        }
                                    })

                                val promptInfo = BiometricPrompt.PromptInfo.Builder()
                                    .setTitle(context.getString(R.string.action_biometric))
                                    .setSubtitle(context.getString(R.string.msg_biometric))
                                    .setAllowedAuthenticators(androidx.biometric.BiometricManager.Authenticators.BIOMETRIC_STRONG or androidx.biometric.BiometricManager.Authenticators.DEVICE_CREDENTIAL)
                                    .build()

                                biometricPrompt.authenticate(promptInfo)
                            } else {
                                // Fallback if no activity context
                                biometricLogin = false
                                prefs.edit().putBoolean("biometric_login", false).apply()
                            }
                        } else {
                            biometricLogin = true
                            prefs.edit().putBoolean("biometric_login", true).apply()
                        }
                    }
                )
            }

            // Strong Biometric
            if (showStrongBiometric) {
                var strongBiometric by remember { mutableStateOf(prefs.getBoolean("strong_biometric", false)) }
                SwitchItem(
                    icon = Icons.Filled.VerifiedUser,
                    title = strongBiometricTitle,
                    summary = strongBiometricSummary,
                    checked = strongBiometric,
                    onCheckedChange = {
                        strongBiometric = it
                        prefs.edit().putBoolean("strong_biometric", it).apply()
                    }
                )
            }

            // Clear SuperKey
            if (showClearSuperKey) {
                 val clearSuperKeyDialog = rememberConfirmDialog(
                    onConfirm = {
                        APatchKeyHelper.clearConfigKey()
                        APApplication.superKey = ""
                    }
                )
                ListItem(
                    colors = ListItemDefaults.colors(containerColor = Color.Transparent),
                    headlineContent = { Text(text = clearSuperKeyTitle) },
                    leadingContent = { Icon(Icons.Filled.KeyOff, null) },
                    modifier = Modifier.clickable {
                        clearSuperKeyDialog.showConfirm(
                            title = clearSuperKeyTitle,
                            content = context.getString(R.string.settings_clear_super_key_dialog)
                        )
                    }
                )
            }

            // Don't Store SuperKey
            if (showNoStoreSuperKey) {
                var noStoreKey by remember { mutableStateOf(APatchKeyHelper.shouldSkipStoreSuperKey()) }
                SwitchItem(
                    icon = Icons.Filled.Key,
                    title = noStoreSuperKeyTitle,
                    summary = noStoreSuperKeySummary,
                    checked = noStoreKey,
                    onCheckedChange = {
                        noStoreKey = it
                        APatchKeyHelper.setShouldSkipStoreSuperKey(it)
                    }
                )
            }
        }
    }
}
