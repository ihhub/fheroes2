import { Button, Grid } from "@mui/material";
import { Header } from "./Header";
import { Settings } from "./Settings/Settings";
import { WallpaperSettings } from "./global";

type Props = {
  settings: WallpaperSettings;
};

export const SettingsPage: React.FC<Props> = ({ settings }) => (
  <>
    <Header title="Wallpaper settings" />

    <Grid
      container
      alignContent="center"
      justifyContent="center"
      flex="1"
      direction="column"
    >
      <Settings settings={settings} />

      <Grid
        container
        alignContent="center"
        justifyContent="flex-start"
        flex="1"
        flexShrink={1}
        padding={2}
        gap={2}
        direction="column"
      >
        <Button
          variant="contained"
          onClick={() => window.Android?.setWallpaper()}
        >
          Set wallpaper
        </Button>
      </Grid>
    </Grid>
  </>
);
