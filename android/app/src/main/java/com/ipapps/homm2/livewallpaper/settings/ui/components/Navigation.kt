package com.ipapps.homm2.livewallpaper.settings.ui.components;
import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import androidx.navigation.NavHostController
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.ipapps.homm2.livewallpaper.settings.data.MapsViewModel
import com.ipapps.homm2.livewallpaper.settings.data.ParsingViewModel
import com.ipapps.homm2.livewallpaper.settings.data.SettingsViewModel

object Destinations {
    const val PARSING = "parsing"
    const val SETTINGS = "settings"
    const val MAPS_LIST = "maps_list"
    const val PHONE_LIMITATION = "phone_limitation"
}

class NavigationActions(navController: NavHostController) {
    val settings: () -> Unit = { navController.navigate(Destinations.SETTINGS) }
    val mapsList: () -> Unit = { navController.navigate(Destinations.MAPS_LIST) }
}

@Composable
fun NavigationHost(
    mapViewModel: MapsViewModel,
    settingsViewModel: SettingsViewModel,
    parsingViewModel: ParsingViewModel,
) {
    val navController = rememberNavController()
    val actions = remember(navController) { NavigationActions(navController) }
    val startDestination = when (parsingViewModel.isGameAssetsAvailable()) {
        true -> Destinations.SETTINGS
        false -> Destinations.PARSING
    }

    NavHost(navController, startDestination) {
        composable(Destinations.PARSING) {
            ParsingScreen(viewModel = parsingViewModel, actions)
        }

        composable(Destinations.PHONE_LIMITATION) {
            PhoneLimitations(actions)
        }

        composable(Destinations.SETTINGS) {
            SettingsScreen(
                viewModel = settingsViewModel,
                actions
            )
        }

        composable(Destinations.MAPS_LIST) {
            MapsScreen(viewModel = mapViewModel)
        }
    }
}