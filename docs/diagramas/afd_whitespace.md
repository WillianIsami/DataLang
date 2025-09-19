```mermaid
graph TD
    q0[q0] --> |space,tab,newline,return| q1((q1_WHITESPACE))
    q1 --> |space,tab,newline,return| q1
```