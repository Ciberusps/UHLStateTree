# 🌿 UHL State Tree

**UHL-StateTree** - Unreal plugin providing tools for **StateTree AI**, similar to [UHL BehaviorTrees nodes](https://github.com/Ciberusps/UHLBehaviorTree). Simplify integration of StateTree into your project with ready-to-use components - tasks/evaluators/conditions/considirations and blueprint libraries.

Support: `UE5.5 (main)`


## ✨ Features

- cooldowns by tag
- InRange and other nodes similar to [UnrealHelperLibrary](https://github.com/Ciberusps/unreal-helper-library) BehaviorTree nodes


## 🚀 Install & ⬆️ Update

#### From source (recommended)

```bash
// install as git submodule to your plugins folder
git submodule add https://github.com/Ciberusps/UHLAI.git ./Plugins/UHLAI
git submodule add https://github.com/Ciberusps/UHLStateTree.git ./Plugins/UHLStateTree

// to update plugin
git submodule update --remote`
```

## 🚀 Setup

TODO how to setup in project


## 📖 Documentation

> - Components
>   - [UHLStateTreeAIComponent](#UUHLStateTreeAIComponent)
> - Conditions
>   - [TagCooldownCondition](#TagCooldownCondition)
>   - [InRange]()
>   - [InAngle]()
> - Evaluators
>   - [TODO: GetGASGamplayTags]()
>   - [TODO: GetSelfGASGamplayTags]() - child of GetGASGamplayTags
>   - [TODO: GetEnemyGASGamplayTags]() - child of GetGASGamplayTags
> - Considers
>   - [TODO: ByHavingTags]()
> - Tasks
>   - [SetCooldownTask](#SetCooldownTask)
>   - [PlayAnimMontage]()
>   - [GameplayFocus]()
>   - [ClearFocus]()
>   - [TurnTo]()
>   - [TODO: ActivateGameplayAbility]()
>   - [TODO: ApplyGameplayEffect]()

### `UUHLStateTreeComponent`

Attach this component to an `AAIController` in Blueprint or C++.
In `UE5.5` there is bug with LinkedStateTreeOverrides, this component adds ability to use LinkedStateTreeOverrides  

## 🤝 Contributing

Contributions welcome! Feel free to open issues or pull requests.

1. Fork repository
2. Create feature branch (`git checkout -b feature/my-feature`)
3. Commit changes (`git commit -am 'Add feature'`)
4. Push branch (`git push origin feature/my-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the **MIT License**. See [LICENSE](LICENSE) for details.
