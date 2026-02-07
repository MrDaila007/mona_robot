import launch
from launch import LaunchDescription
from launch_ros.actions import LifecycleNode
from launch.actions import RegisterEventHandler, EmitEvent
from launch.event_handlers import OnProcessStart
from launch_ros.events.lifecycle import ChangeState
from launch_ros.event_handlers import OnStateTransition
import lifecycle_msgs.msg

def generate_launch_description():
    # 1. Объявляем нашу C++ ноду
    safety_node = LifecycleNode(
        package='mona_safety',
        executable='mona_safety_node', 
        name='safety_node',
        namespace='',
    )

    # 2. Событие: АВТО-КОНФИГУРАЦИЯ
    # Как только процесс запустился (OnProcessStart), мы шлем запрос "configure"
    configure_event = RegisterEventHandler(
        event_handler=OnProcessStart(
            target_action=safety_node,
            on_start=[
                EmitEvent(
                    event=ChangeState(
                        lifecycle_node_matcher=launch.events.matches_action(safety_node),
                        transition_id=lifecycle_msgs.msg.Transition.TRANSITION_CONFIGURE,
                    )
                )
            ],
        )
    )

    # 3. Событие: АВТО-АКТИВАЦИЯ
    # Как только нода перешла в состояние 'inactive', шлем запрос "activate"
    activate_event = RegisterEventHandler(
        event_handler=OnStateTransition(
            target_lifecycle_node=safety_node,
            goal_state='inactive',
            entities=[
                EmitEvent(
                    event=ChangeState(
                        lifecycle_node_matcher=launch.events.matches_action(safety_node),
                        transition_id=lifecycle_msgs.msg.Transition.TRANSITION_ACTIVATE,
                    )
                )
            ],
        )
    )

    return LaunchDescription([
        safety_node,
        configure_event,
        activate_event,
    ])