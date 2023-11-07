import { Grid, List } from "@mui/material";
import { useBridgeContext } from "./BridgeContext";
import { Header } from "./Header";
import { MapItem } from "./MapsList/MapItem";
import { Upload } from "./MapsList/Upload";

export const MapsListPage = () => {
  const { mapsList } = useBridgeContext();

  return (
    <>
      <Header title="Maps list" backButton />

      <Grid container alignContent="center" justifyContent="center" padding={2}>
        <Upload />
      </Grid>

      <Grid container flex="1" direction="column">
        <List disablePadding dense>
          {mapsList.map((item) => (
            <MapItem
              key={item.filename}
              map={item}
              deletable={mapsList.length > 1}
            />
          ))}
        </List>
      </Grid>
    </>
  );
};
