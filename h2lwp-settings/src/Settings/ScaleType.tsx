import { useMemo, useState } from "react";
import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: string;
};

const valueToText: Record<string, string> = {
  NEAREST: "Nearest",
  LINEAR: "Linear",
};

export const ScaleType: React.FC<Props> = ({ value: initialValue }) => {
  const items = useMemo(() => {
    return Object.entries(valueToText).map(([value, title]) => ({
      value,
      title,
    }));
  }, []);

  const [value, setValue] = useState(initialValue);

  return (
    <SimpleListMenu
      label="Scale type"
      value={value}
      items={items}
      onChange={setValue}
    />
  );
};
