#pragma once
#include "src/core/StateMachine/StateMachine.h"
#include "src/core/Window.h"
#include "src/core/Components/ButtonComponent.h"

class MenuState : public State
{
public:
    void OnEnter() override;
    void ShowConnectionButton();
    void OnUpdate(float dt) override;
    void OnExit() override;

    MenuState(StateMachine* stateMachine, Window* window);
    MenuState(const MenuState& other) = delete;
    MenuState& operator=(const MenuState& other) = delete;
    ~MenuState() override;

private:
    Window* m_Window = nullptr;

    ButtonComponent* m_ConnectButton = nullptr;
    ButtonComponent* m_QuitButton = nullptr;
    ButtonComponent* m_DisconnectButton = nullptr;
};
