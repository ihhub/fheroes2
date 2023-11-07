import DeleteIcon from "@mui/icons-material/Delete";
import DraftsIcon from "@mui/icons-material/Drafts";
import { IconButton, ListItem, ListItemText } from "@mui/material";
import ListItemIcon from "@mui/material/ListItemIcon";
import { useBridgeContext } from "../BridgeContext";
import { WallpaperMapItem } from "../global";

type Props = {
  map: WallpaperMapItem;
  deletable?: boolean;
};

export const MapItem: React.FC<Props> = ({ map, deletable }) => {
  const { androidInterface } = useBridgeContext();

  return (
    <ListItem
      secondaryAction={
        deletable && (
          <IconButton
            edge="end"
            aria-label="delete"
            onClick={() => androidInterface?.deleteMap(map.filename)}
          >
            <DeleteIcon />
          </IconButton>
        )
      }
    >
      {map.isPoL && (
        <ListItemIcon>
          <DraftsIcon />
        </ListItemIcon>
      )}

      <ListItemText
        primary={map.filename}
        secondary={`(${map.width}x${map.height}) ${map.title}`}
      />
    </ListItem>
  );
};
