import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: number;
};

const valueToText: Record<number, string> = {
  0: "DPI",
  1: "x1",
  2: "x2",
  3: "x3",
  4: "x4",
  5: "x5",
};

const items = Object.entries(valueToText).map(([value, title]) => ({
  value: Number(value),
  title,
}));

export const Scale: React.FC<Props> = ({ value }) => (
  <SimpleListMenu
    label="Scale"
    value={value}
    items={items}
    onChange={(value) => window.Android?.setScale(value)}
  />
);
