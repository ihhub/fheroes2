import { Grid, List } from "@mui/material";
import { useEffect, useState } from "react";
import { Header } from "./Header";
import { MapItem } from "./MapsList/MapItem";
import { Upload } from "./MapsList/Upload";
import { WallpaperMapItem } from "./global";

type Props = {};

export const MapsListPage: React.FC<Props> = () => {
  const [list, setList] = useState<WallpaperMapItem[]>([]);

  useEffect(() => {
    window.Android?.getMapsList()
      .then(JSON.parse)
      .then(setList)
      .catch((e) => console.log("getMapsList err", e));
  }, []);

  console.log(list);

  return (
    <>
      <Header title="Maps list" />

      <Upload />

      <Grid container flex="1" direction="column">
        <List disablePadding dense>
          {list.map((item) => (
            <MapItem key={item.name} map={item} />
          ))}
        </List>
      </Grid>
    </>
  );
};
