import DraftsIcon from "@mui/icons-material/Drafts";
import { Button, List, ListItem, ListItemText, styled } from "@mui/material";
import ListItemIcon from "@mui/material/ListItemIcon";
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
    window.Android?.getMapsList()
      .then(JSON.parse)
      .then(setList)
      .catch((e) => console.log("getMapsList err", e));
  }, []);

  return (
    <List sx={{ width: "100%" }} disablePadding dense>
      {list.map((item) => (
        <ListItem key={item.name}>
          {item.isPoL && (
            <ListItemIcon>
              <DraftsIcon />
            </ListItemIcon>
          )}
          <ListItemText
            primary={item.name}
            secondary={`(${item.width}x${item.height}) isPoL: ${item.isPoL} ${item.title}`}
          />
        </ListItem>
      ))}

      <Button variant="contained" component="label">
        Upload
        <VisuallyHiddenInput
          type="file"
          onChange={(e) => {
            if (Boolean(e.currentTarget?.files?.length)) {
              console.log("file is MP2", e);
            } else {
              console.log("file is PoL", e);
            }
          }}
        />
      </Button>
    </List>
  );
};
