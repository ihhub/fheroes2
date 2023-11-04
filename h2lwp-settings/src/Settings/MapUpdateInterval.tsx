import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: number;
};

const valueToText: Record<number, string> = {
  0: "Every switch to home screen",
  1: "10 minutes",
  2: "30 minutes",
  3: "2 hour",
  4: "24 hours",
};

const items = Object.entries(valueToText).map(([value, title]) => ({
  value: Number(value),
  title,
}));

export const MapUpdateInterval: React.FC<Props> = ({ value }) => (
  <SimpleListMenu
    label="Map update interval"
    value={value}
    items={items}
    onChange={(value) => window.Android?.setMapUpdateInterval(value)}
  />
);
