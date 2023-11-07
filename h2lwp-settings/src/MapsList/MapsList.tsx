import { Grid, List } from "@mui/material";
import { useEffect, useState } from "react";
import { Header } from "../Header";
import { WallpaperMapItem } from "../global";
import { MapItem } from "./MapItem";
import { Upload } from "./Upload";

type Props = {};

export const MapsList: React.FC<Props> = () => {
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
      <Header title="Wallpaper settings" />

      <Upload />

      <Grid
        container
        alignContent="center"
        justifyContent="center"
        flex="1"
        direction="column"
      >
        <List sx={{ width: "100%" }} disablePadding dense>
          {list.map((item) => (
            <MapItem key={item.name} map={item} />
          ))}
        </List>
      </Grid>
    </>
  );
};
