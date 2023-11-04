import { useMemo, useState } from "react";
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

export const Scale: React.FC<Props> = () => {
  const items = useMemo(() => {
    return Object.entries(valueToText).map(([value, title]) => ({
      value,
      title,
    }));
  }, []);

  const [value, setValue] = useState("");

  return (
    <SimpleListMenu
      label="Select scale"
      value={value}
      items={items}
      onChange={setValue}
    />
  );
};
