import { Button, Grid } from "@mui/material";
import { useState } from "react";
import { useBridgeContext } from "../BridgeContext";
import { LimitationsDialog } from "./LimitationsDialog";

export const SetWallpaper = () => {
  const { androidInterface } = useBridgeContext();
  const [isAlert, setIsAlert] = useState(false);

  return (
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
      <LimitationsDialog
        open={isAlert}
        onClose={() => setIsAlert(false)}
        onConfirm={() => {
          setIsAlert(false);
          androidInterface?.setWallpaper();
        }}
      />

      <Button variant="contained" onClick={() => setIsAlert(true)}>
        Set wallpaper
      </Button>
    </Grid>
  );
};
