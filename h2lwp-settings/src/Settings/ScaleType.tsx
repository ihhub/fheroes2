import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: number;
};

const valueToText = {
  0: "Nearest",
  1: "Linear",
} as const;

const items = Object.entries(valueToText).map(([value, title]) => ({
  value: Number(value),
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
