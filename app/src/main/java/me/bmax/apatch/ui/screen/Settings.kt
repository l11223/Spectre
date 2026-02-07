package me.bmax.apatch.ui.screen

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.livedata.observeAsState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import com.ramcosta.composedestinations.annotation.Destination
import com.ramcosta.composedestinations.annotation.RootGraph
import com.ramcosta.composedestinations.navigation.DestinationsNavigator
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import me.bmax.apatch.APApplication
import me.bmax.apatch.R
import me.bmax.apatch.ui.component.SearchAppBar
import me.bmax.apatch.ui.screen.settings.*
import me.bmax.apatch.util.APatchKeyHelper
import me.bmax.apatch.util.getSELinuxMode
import me.bmax.apatch.util.isGlobalNamespaceEnabled as checkGlobalNamespaceEnabled
import me.bmax.apatch.util.isMagicMountEnabled as checkMagicMountEnabled
import me.bmax.apatch.util.ui.LocalSnackbarHost

import com.ramcosta.composedestinations.generated.destinations.ThemeStoreScreenDestination

@Destination<RootGraph>
@Composable
@OptIn(ExperimentalMaterial3Api::class)
fun SettingScreen(navigator: DestinationsNavigator) {
    val state by APApplication.apStateLiveData.observeAsState(APApplication.State.UNKNOWN_STATE)
    val kPatchReady = state != APApplication.State.UNKNOWN_STATE
    val aPatchReady =
        (state == APApplication.State.ANDROIDPATCH_INSTALLING || state == APApplication.State.ANDROIDPATCH_INSTALLED || state == APApplication.State.ANDROIDPATCH_NEED_UPDATE)
    
    var isGlobalNamespaceEnabled by rememberSaveable { mutableStateOf(false) }
    var isMagicMountEnabled by rememberSaveable { mutableStateOf(false) }
    var currentSELinuxMode by rememberSaveable { mutableStateOf("Unknown") }
    var searchText by rememberSaveable { mutableStateOf("") }
    
    // Auto Backup Module State (Lifted)
    val prefs = APApplication.sharedPreferences
    var autoBackupModule by rememberSaveable { mutableStateOf(prefs.getBoolean("auto_backup_module", false)) }

    LaunchedEffect(kPatchReady, aPatchReady) {
        if (kPatchReady && aPatchReady) {
            withContext(Dispatchers.IO) {
                val globalNamespace = checkGlobalNamespaceEnabled()
                val magicMount = checkMagicMountEnabled()
                val seLinux = getSELinuxMode()
                isGlobalNamespaceEnabled = globalNamespace
                isMagicMountEnabled = magicMount
                currentSELinuxMode = seLinux
            }
        }
    }

    val snackBarHost = LocalSnackbarHost.current

    Scaffold(
        topBar = {
            SearchAppBar(
                title = { Text(stringResource(R.string.settings)) },
                searchText = searchText,
                onSearchTextChange = { searchText = it },
                onClearClick = { searchText = "" },
                dropdownContent = {}
            )
        },
        containerColor = Color.Transparent,
        snackbarHost = { SnackbarHost(snackBarHost) }
    ) { paddingValues ->

        Column(
            modifier = Modifier
                .padding(paddingValues)
                .verticalScroll(rememberScrollState()),
        ) {
            GeneralSettings(
                searchText = searchText,
                kPatchReady = kPatchReady,
                aPatchReady = aPatchReady,
                currentSELinuxMode = currentSELinuxMode,
                onSELinuxModeChange = { currentSELinuxMode = it },
                isGlobalNamespaceEnabled = isGlobalNamespaceEnabled,
                onGlobalNamespaceChange = { isGlobalNamespaceEnabled = it },
                isMagicMountEnabled = isMagicMountEnabled,
                onMagicMountChange = { isMagicMountEnabled = it },
                snackBarHost = snackBarHost
            )

            AppearanceSettings(
                searchText = searchText,
                snackBarHost = snackBarHost,
                kPatchReady = kPatchReady,
                onNavigateToThemeStore = {
                    navigator.navigate(ThemeStoreScreenDestination)
                }
            )

            BehaviorSettings(
                searchText = searchText,
                kPatchReady = kPatchReady,
                aPatchReady = aPatchReady
            )

            SecuritySettings(
                searchText = searchText,
                snackBarHost = snackBarHost,
                kPatchReady = kPatchReady
            )

            if (aPatchReady) {
                BackupSettings(
                    searchText = searchText,
                    autoBackupModule = autoBackupModule,
                    onAutoBackupModuleChange = { autoBackupModule = it }
                )
            }


            if (aPatchReady) {
                ModuleSettings(
                    searchText = searchText,
                    aPatchReady = aPatchReady
                )
            }

            MultimediaSettings(
                searchText = searchText,
                snackBarHost = snackBarHost
            )
        }
    }
}
