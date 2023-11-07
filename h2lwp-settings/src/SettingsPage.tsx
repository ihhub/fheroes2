import { Button, Grid } from "@mui/material";
import { useBridgeContext } from "./BridgeContext";
import { Header } from "./Header";
import { Settings } from "./Settings/Settings";

export const SettingsPage = () => {
  const { androidInterface } = useBridgeContext();

  return (
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
            onClick={() => androidInterface?.setWallpaper()}
          >
            Set wallpaper
          </Button>
        </Grid>
      </Grid>
    </>
  );
};
