```mermaid
graph TD
    q0[q0] --> |"| q1
    q1 --> |any character except " or \ | q1
    q1 --> |\\| q2
    q1 --> |"| q3((q3_STRING))
    q2 --> |" , \\ , n , t , r| q1
```
