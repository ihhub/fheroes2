import { List } from "@mui/material";
import { useEffect, useState } from "react";
import { WallpaperMapItem } from "../global";
import { MapItem } from "./MapItem";

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
    <List sx={{ width: "100%" }} disablePadding dense>
      {list.map((item) => (
        <MapItem key={item.name} map={item} />
      ))}
    </List>
  );
};
