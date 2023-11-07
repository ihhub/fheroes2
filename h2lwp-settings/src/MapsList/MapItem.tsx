import DeleteIcon from "@mui/icons-material/Delete";
import DraftsIcon from "@mui/icons-material/Drafts";
import { IconButton, ListItem, ListItemText } from "@mui/material";
import ListItemIcon from "@mui/material/ListItemIcon";
import { useCallback } from "react";
import { WallpaperMapItem } from "../global";

type Props = {
  map: WallpaperMapItem;
};

export const MapItem: React.FC<Props> = ({ map }) => {
  const handleDelete = useCallback(() => {
    window.Android?.deleteMap(map.name);
  }, []);

  return (
    <ListItem
      secondaryAction={
        <IconButton edge="end" aria-label="delete" onClick={handleDelete}>
          <DeleteIcon />
        </IconButton>
      }
    >
      {map.isPoL && (
        <ListItemIcon>
          <DraftsIcon />
        </ListItemIcon>
      )}

      <ListItemText
        primary={map.name}
        secondary={`(${map.width}x${map.height}) ${map.title}`}
      />
    </ListItem>
  );
};
