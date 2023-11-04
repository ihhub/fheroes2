import { useEffect, useMemo, useState } from "react";
import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: number;
};

const valueToText = {
  0: "Nearest",
  1: "Linear",
} as const;

export const ScaleType: React.FC<Props> = ({ value: initialValue }) => {
  const items = useMemo(() => {
    return Object.entries(valueToText).map(([value, title]) => ({
      value: Number(value),
      title,
    }));
  }, []);

  const [value, setValue] = useState(initialValue);
  useEffect(() => {
    window.Android?.setScaleType(value);
  }, [value]);

  return (
    <SimpleListMenu
      label="Scale type"
      value={value}
      items={items}
      onChange={setValue}
    />
  );
};
