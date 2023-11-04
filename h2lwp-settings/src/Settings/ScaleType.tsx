import { useEffect, useMemo, useState } from "react";
import { SimpleListMenu } from "./SimpleListMenu";

type Props = {
  value: string;
};

const valueToText = {
  NEAREST: "Nearest",
  LINEAR: "Linear",
} as const;

const getInitialValue = (value: string) => {
  if (value === "NEAREST") {
    return "NEAREST" as const;
  } else {
    return "LINEAR" as const;
  }
};

export const ScaleType: React.FC<Props> = ({ value: initialValue }) => {
  const items = useMemo(() => {
    return Object.entries(valueToText).map(([value, title]) => ({
      value: value as keyof typeof valueToText,
      title,
    }));
  }, []);

  const [value, setValue] = useState(getInitialValue(initialValue));
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
