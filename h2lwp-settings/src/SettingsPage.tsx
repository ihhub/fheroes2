import { Grid } from "@mui/material";
import { Header } from "./Header";
import { SetWallpaper } from "./Settings/SetWallpaper";
import { Settings } from "./Settings/Settings";

export const SettingsPage = () => (
  <>
    <Header title="Wallpaper settings" />

    <Grid
      container
      alignContent="center"
      justifyContent="center"
      flex="1"
      direction="column"
    >
      <Settings />

      <SetWallpaper />
    </Grid>
  </>
);
