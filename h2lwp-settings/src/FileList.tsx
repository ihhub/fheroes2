import { Button, List, ListItem, ListItemText, styled } from "@mui/material";
import { useEffect, useState } from "react";
import { WallpaperMapItem } from "./global";

const VisuallyHiddenInput = styled("input")({
  clip: "rect(0 0 0 0)",
  clipPath: "inset(50%)",
  height: 1,
  overflow: "hidden",
  position: "absolute",
  bottom: 0,
  left: 0,
  whiteSpace: "nowrap",
  width: 1,
});

type Props = {};

export const FileList: React.FC<Props> = () => {
  const [list, setList] = useState<WallpaperMapItem[]>([]);

  useEffect(() => {
    console.log("getMapsList start");
    window.Android?.getMapsList()
      .then(JSON.parse)
      .then((result) => {
        console.log("getMapsList", result);
        setList(result);
      })
      .catch((e) => console.log("getMapsList err", e));
  }, []);

  return (
    <List sx={{ width: "100%" }} disablePadding dense>
      {list.map((item) => (
        <ListItem key={item.name}>
          <ListItemText
            primary={item.title}
            secondary={`(${item.width}x${item.height}) ${item.name}`}
          />
        </ListItem>
      ))}

      <Button variant="contained" component="label">
        Upload
        <VisuallyHiddenInput
          type="file"
          onChange={(e) => console.log("on change", e)}
        />
      </Button>
    </List>
  );
};
