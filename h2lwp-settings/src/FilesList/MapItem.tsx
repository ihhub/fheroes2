import DraftsIcon from "@mui/icons-material/Drafts";
import { ListItem, ListItemText } from "@mui/material";
import ListItemIcon from "@mui/material/ListItemIcon";
import { WallpaperMapItem } from "../global";

type Props = {
  map: WallpaperMapItem;
};

export const MapItem: React.FC<Props> = ({ map }) => (
  <ListItem>
    {map.isPoL && (
      <ListItemIcon>
        <DraftsIcon />
      </ListItemIcon>
    )}

    <ListItemText
      primary={map.name}
      secondary={`(${map.width}x${map.height}) isPoL: ${map.isPoL} ${map.title}`}
    />
  </ListItem>
);
