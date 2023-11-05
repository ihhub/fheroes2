import { WallpaperScaleType } from "../global";
import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: WallpaperScaleType;
};

const valueToText: Record<WallpaperScaleType, string> = {
  0: "Nearest",
  1: "Linear",
};

const items = Object.entries(valueToText).map(([value, title]) => ({
  value: Number(value) as WallpaperScaleType,
  title,
}));

export const ScaleType: React.FC<Props> = ({ value }) => (
  <SimpleListMenu
    label="Scale type"
    value={value}
    items={items}
    onChange={(value) => window.Android?.setScaleType(value)}
  />
);
