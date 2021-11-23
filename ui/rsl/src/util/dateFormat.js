import { format } from "date-fns";

export function formatDateTime(ts) {
  return format(new Date(ts * 1000), "dd.MM.yyyy, HH:mm:ss O");
}

export function formatTime(ts) {
  return format(new Date(ts * 1000), "HH:mm");
}

export function formatFileNameTime(ts) {
  return format(new Date(ts * 1000), "yyyy-MM-dd_HHmm");
}
