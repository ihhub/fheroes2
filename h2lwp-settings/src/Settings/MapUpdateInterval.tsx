import { useBridgeContext } from "../BridgeContext";
import { WallpaperMapUpdateInterval } from "../global";
import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: WallpaperMapUpdateInterval;
};

const valueToText: Record<WallpaperMapUpdateInterval, string> = {
  0: "Every switch to home screen",
  1: "10 minutes",
  2: "30 minutes",
  3: "2 hour",
  4: "24 hours",
};

const items = Object.entries(valueToText).map(([value, title]) => ({
  value: Number(value) as WallpaperMapUpdateInterval,
  title,
}));

export const MapUpdateInterval: React.FC<Props> = ({ value }) => {
  const { androidInterface } = useBridgeContext();

  return (
    <SimpleListMenu
      label="Map update interval"
      value={value}
      items={items}
      onChange={(value) => androidInterface?.setMapUpdateInterval(value)}
    />
  );
};
