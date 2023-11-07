import { Grid, List } from "@mui/material";
import { Header } from "./Header";
import { MapItem } from "./MapsList/MapItem";
import { Upload } from "./MapsList/Upload";
import { WallpaperMapItem } from "./global";

type Props = {
  list: WallpaperMapItem[];
};

export const MapsListPage: React.FC<Props> = ({ list }) => (
  <>
    <Header title="Maps list" />

    <Grid container alignContent="center" justifyContent="center" padding={2}>
      <Upload />
    </Grid>

    <Grid container flex="1" direction="column">
      <List disablePadding dense>
        {list.map((item) => (
          <MapItem key={item.filename} map={item} />
        ))}
      </List>
    </Grid>
  </>
);
