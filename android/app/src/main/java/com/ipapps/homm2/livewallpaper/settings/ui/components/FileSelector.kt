package com.ipapps.homm2.livewallpaper.settings.ui.components;

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.widget.Toast
import androidx.activity.compose.ManagedActivityResultLauncher
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.runtime.Composable
import androidx.compose.ui.platform.LocalContext
import androidx.core.app.ActivityCompat
import com.ipapps.homm2.livewallpaper.R

const val mimeType = "application/octet-stream"

class GetFile : ActivityResultContracts.GetContent() {
    override fun createIntent(context: Context, input: String): Intent {
        super.createIntent(context, input)

        return Intent(Intent.ACTION_GET_CONTENT)
            .setType(mimeType)
            .addCategory(Intent.CATEGORY_OPENABLE)
            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            .putExtra(Intent.EXTRA_LOCAL_ONLY, true)
            .putExtra(Intent.EXTRA_ALLOW_MULTIPLE, false)
    }
}

@Composable
fun permissionGrant(onGrant: (isGranted: Boolean) -> Unit): ManagedActivityResultLauncher<String, Boolean> {
    return rememberLauncherForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) {
        onGrant(it)
    }
}

@Composable
fun createFileSelector(onSelect: (uri: Uri) -> Unit): () -> Unit {
    val context = LocalContext.current
    val filesSelector =
        rememberLauncherForActivityResult(GetFile()) { list ->
            if (list !== null) {
                onSelect(list)
            }
        }

    val requestPermission = permissionGrant {
        if (it) {
            filesSelector.launch(mimeType)
        } else {
            Toast
                .makeText(
                    context,
                    context.getText(R.string.file_selector_toast),
                    Toast.LENGTH_LONG
                )
                .show()
        }
    }

    return fun() {
        val hasPermission = ActivityCompat.checkSelfPermission(
            context,
            Manifest.permission.READ_EXTERNAL_STORAGE
        ) == PackageManager.PERMISSION_GRANTED

        if (hasPermission) {
            filesSelector.launch(mimeType)
        } else {
            requestPermission.launch(Manifest.permission.READ_EXTERNAL_STORAGE)
        }
    }
}